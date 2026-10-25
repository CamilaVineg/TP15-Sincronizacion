#pragma once

// Cliente del método de Cristian
// Envía una petición al servidor con T0 (su reloj), recibe respuesta con T_recv/T_send
// Calcula RTT, estima T_nuevo = T_send + RTT_neto/2 y ajusta su reloj local.

#include "cristian_common.hpp"

namespace cristian {

class CristianClient {
private:
    std::string server_ip;
    uint16_t server_port;
    LocalClock clock; // Reloj local simulado (puede estar atrasado/adelantado)

public:
    CristianClient(std::string ip = "127.0.0.1", uint16_t port = DEFAULT_PORT, int64_t initial_drift_ms = 4500)
        : server_ip(std::move(ip)), server_port(port), clock(initial_drift_ms) {}

    LocalClock& get_clock() { return clock; }

    // Ejecuta una sincronización individual siguiendo el método de Cristian
    // apply_adjustment: si true, aplica el offset calculado al reloj local
    // use_slewing: si true y el reloj está adelantado (offset < 0), usa ajuste gradual para no retroceder
    // estimated_tmin: estimado mínimo de latencia en un sentido (para calcular cota de error)
    bool sync_once(bool apply_adjustment = true, bool use_slewing = false, int64_t estimated_tmin = 1) {
        if (!socket_init()) {
            std::cerr << "[ERROR] Falló la inicialización de sockets del sistema." << std::endl;
            return false;
        }

        socket_t sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (sock == INVALID_SOCKET_FD) {
            std::cerr << "[ERROR] No se pudo crear socket UDP." << std::endl;
            socket_cleanup();
            return false;
        }

        // Configurar timeout de recepción (3 segundos)
#ifdef _WIN32
        DWORD timeout = 3000;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
#else
        struct timeval tv;
        tv.tv_sec = 3;
        tv.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif

        sockaddr_in srv_addr{};
        srv_addr.sin_family = AF_INET;
        srv_addr.sin_port = htons(server_port);
        inet_pton(AF_INET, server_ip.c_str(), &srv_addr.sin_addr);

        Packet req{};
        req.magic = PROTOCOL_MAGIC;
        req.type = MSG_REQUEST;
        
        // T0: Momento en que el cliente envía la petición (según su reloj local)
        int64_t t0 = clock.now_ms();
        req.t0_client_send = t0;

        int sent = sendto(sock, reinterpret_cast<const char*>(&req), sizeof(req), 0,
                          reinterpret_cast<sockaddr*>(&srv_addr), sizeof(srv_addr));
        if (sent <= 0) {
            std::cerr << "[ERROR] Falló el envío del paquete al servidor " << server_ip << ":" << server_port << std::endl;
            close_socket(sock);
            socket_cleanup();
            return false;
        }

        Packet resp{};
        sockaddr_in from_addr{};
        socklen_val_t from_len = sizeof(from_addr);

        // Esperamos la respuesta del servidor
        int recvd = recvfrom(sock, reinterpret_cast<char*>(&resp), sizeof(resp), 0,
                             reinterpret_cast<sockaddr*>(&from_addr), &from_len);

        // T1: Momento en que el cliente recibe la respuesta (según su reloj local)
        int64_t t1 = clock.now_ms();

        close_socket(sock);
        socket_cleanup();

        if (recvd <= 0 || resp.magic != PROTOCOL_MAGIC || resp.type != MSG_RESPONSE) {
            std::cerr << "[ERROR] Tiempo de espera agotado o respuesta inválida del servidor." << std::endl;
            return false;
        }

        resp.t1_client_recv = t1;
        // Aplicamos el algoritmo de Cristian con los cuatro tiempos
        SyncResult sr = calculate_cristian(t0, resp.t_server_recv, resp.t_server_send, t1, estimated_tmin);

        print_sync_report(sr);

        if (apply_adjustment) {
            std::cout << "\n>>> APLICANDO AJUSTE DE RELOJ AL CLIENTE <<<" << std::endl;
            std::cout << "  Reloj antes del ajuste:  " << format_time_ms(clock.now_ms()) << std::endl;
            if (use_slewing && sr.offset < 0) {
                // Offset negativo = reloj del cliente adelantado -> para no retrocederlo, usamos ajuste gradual
                std::cout << "  Aviso: Offset negativo detectado (reloj adelantado). Aplicando slewing para no retroceder el reloj..." << std::endl;
                clock.slew_adjust(sr.offset, 3);
            } else {
                // Offset positivo o cero, o sin slewing: ajuste directo por salto
                clock.step_adjust(sr.offset);
            }
            std::cout << "  Reloj después del ajuste: " << format_time_ms(clock.now_ms()) << std::endl;
            std::cout << "  Nuevo desfase estimado:   " << clock.get_current_offset() << " ms" << std::endl;
        }

        return true;
    }

    // Variante: enviar varias peticiones y quedarse con la que tenga menor RTT neto
    // Esto reduce el error porque el menor RTT significa menor tiempo en colas/asimetrías
    void sync_best_of_n(int num_samples = 5, int64_t estimated_tmin = 1) {
        std::cout << "\n========================================================" << std::endl;
        std::cout << "   ALGORITMO DE CRISTIAN CON FILTRO DE MÚLTIPLES MUESTRAS" << std::endl;
        std::cout << "   (Enviando " << num_samples << " peticiones para seleccionar el menor RTT)" << std::endl;
        std::cout << "========================================================\n" << std::endl;

        std::vector<SyncResult> samples;

        for (int i = 1; i <= num_samples; ++i) {
            std::cout << "--- [Muestra " << i << " de " << num_samples << "] ---" << std::endl;
            if (!socket_init()) continue;

            socket_t sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            if (sock == INVALID_SOCKET_FD) {
                socket_cleanup();
                continue;
            }

#ifdef _WIN32
            DWORD timeout = 2000;
            setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
#else
            struct timeval tv{2, 0};
            setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif

            sockaddr_in srv_addr{};
            srv_addr.sin_family = AF_INET;
            srv_addr.sin_port = htons(server_port);
            inet_pton(AF_INET, server_ip.c_str(), &srv_addr.sin_addr);

            Packet req{};
            req.magic = PROTOCOL_MAGIC;
            req.type = MSG_REQUEST;

            int64_t t0 = clock.now_ms();
            req.t0_client_send = t0;

            sendto(sock, reinterpret_cast<const char*>(&req), sizeof(req), 0,
                   reinterpret_cast<sockaddr*>(&srv_addr), sizeof(srv_addr));

            Packet resp{};
            sockaddr_in from_addr{};
            socklen_val_t from_len = sizeof(from_addr);

            int recvd = recvfrom(sock, reinterpret_cast<char*>(&resp), sizeof(resp), 0,
                                 reinterpret_cast<sockaddr*>(&from_addr), &from_len);

            int64_t t1 = clock.now_ms();
            close_socket(sock);
            socket_cleanup();

            if (recvd > 0 && resp.magic == PROTOCOL_MAGIC) {
                SyncResult sr = calculate_cristian(t0, resp.t_server_recv, resp.t_server_send, t1, estimated_tmin);
                samples.push_back(sr);
                std::cout << "  Muestra recibida con exito: RTT_net = " << sr.rtt_net 
                          << " ms | Offset calculado = " << sr.offset 
                          << " ms | Cota de error = +-" << std::fixed << std::setprecision(1) << sr.error_bound << " ms\n";
            } else {
                std::cout << "  Muestra fallida o descartada por timeout.\n";
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }

        if (samples.empty()) {
            std::cerr << "[ERROR] No se obtuvieron respuestas válidas del servidor." << std::endl;
            return;
        }

        // Seleccionamos la muestra con menor RTT neto: esto suele dar menor incertidumbre
        auto best_it = std::min_element(samples.begin(), samples.end(),
                                        [](const SyncResult& a, const SyncResult& b) {
                                            return a.rtt_net < b.rtt_net;
                                        });

        std::cout << "\n========================================================" << std::endl;
        std::cout << "   RESULTADO DE LA MEJOR MUESTRA SELECCIONADA" << std::endl;
        std::cout << "   (Criterio: menor RTT neto -> menor error estimado)" << std::endl;
        std::cout << "========================================================" << std::endl;
        print_sync_report(*best_it);

        std::cout << "\n>>> AJUSTANDO RELOJ CON LA MEJOR MUESTRA (Min RTT: " << best_it->rtt_net << " ms) <<<" << std::endl;
        clock.step_adjust(best_it->offset);
        std::cout << "  Reloj sincronizado: " << format_time_ms(clock.now_ms()) << std::endl;
    }

private:
    // Imprime el paso a paso del cálculo para entenderlo fácilmente
    void print_sync_report(const SyncResult& sr) {
        std::cout << "\n--------------------------------------------------------\n"
                  << "  INFORME DETALLADO - MÉTODO DE CRISTIAN\n"
                  << "--------------------------------------------------------\n"
                  << "  1. Marcas temporales registradas:\n"
                  << "     - T0 (Envío cliente):          " << format_time_ms(sr.t0) << " (" << sr.t0 << " ms)\n"
                  << "     - T_recv (Llegada a servidor): " << format_time_ms(sr.t_server_recv) << " (" << sr.t_server_recv << " ms)\n"
                  << "     - T_send (Respuesta servidor): " << format_time_ms(sr.t_server_send) << " (" << sr.t_server_send << " ms)\n"
                  << "     - T1 (Recepción cliente):      " << format_time_ms(sr.t1) << " (" << sr.t1 << " ms)\n"
                  << "\n"
                  << "  2. Parámetros de red calculados:\n"
                  << "     - RTT Total (T1 - T0):         " << sr.rtt_total << " ms\n"
                  << "     - Tiempo procesamiento servidor (I): " << sr.proc_time << " ms\n"
                  << "     - RTT Neto (RTT - I):          " << sr.rtt_net << " ms\n"
                  << "     - Latencia estimada en 1 vía (d = RTT/2): " << sr.one_way_delay << " ms\n"
                  << "\n"
                  << "  3. Estimación y Sincronización:\n"
                  << "     - T_nuevo = T_server + (RTT_net / 2): " << format_time_ms(sr.t_new) << " (" << sr.t_new << " ms)\n"
                  << "     - Desfase (Offset = T_nuevo - T1):    " << sr.offset << " ms\n"
                  << "\n"
                  << "  4. Precisión Teórica (según Cristian):\n"
                  << "     - Tiempo mínimo propagación T_min:   " << sr.t_min << " ms\n"
                  << "     - Rango de tiempo real en T1:        [" << sr.t_server_send + sr.t_min 
                  << " ms, " << sr.t_server_send + sr.rtt_net - sr.t_min << " ms]\n"
                  << "     - Cota de error máxima:              +-" << std::fixed << std::setprecision(2) << sr.error_bound << " ms\n"
                  << "--------------------------------------------------------" << std::endl;
    }
};

} // namespace cristian
