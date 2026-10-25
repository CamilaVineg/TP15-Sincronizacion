#pragma once

// Servidor de tiempo para el método de Cristian
// Tiene un "reloj autoritativo" (usamos el reloj del sistema) y responde a las peticiones
// devolviendo T_recv y T_send para que el cliente calcule RTT y haga la estimación.

#include "cristian_common.hpp"
#include <atomic>

namespace cristian {

class CristianServer {
private:
    uint16_t port;
    std::atomic<bool> running{false};
    socket_t server_sock{INVALID_SOCKET_FD};

public:
    explicit CristianServer(uint16_t p = DEFAULT_PORT) : port(p) {}

    ~CristianServer() {
        stop();
    }

    void stop() {
        running = false;
        if (server_sock != INVALID_SOCKET_FD) {
            close_socket(server_sock);
            server_sock = INVALID_SOCKET_FD;
        }
    }

    void run() {
        if (!socket_init()) {
            std::cerr << "[ERROR] Falló la inicialización de sockets del sistema." << std::endl;
            return;
        }

        server_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (server_sock == INVALID_SOCKET_FD) {
            std::cerr << "[ERROR] No se pudo crear el socket UDP." << std::endl;
            socket_cleanup();
            return;
        }

        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(port);

        if (bind(server_sock, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == SOCKET_ERROR_VAL) {
            std::cerr << "[ERROR] Falló el bind en el puerto " << port << "." << std::endl;
            close_socket(server_sock);
            socket_cleanup();
            return;
        }

        running = true;
        std::cout << "========================================================\n"
                  << "   SERVIDOR DE TIEMPO DE CRISTIAN ACTIVO\n"
                  << "   Puerto UDP: " << port << "\n"
                  << "   Fuente de reloj: Reloj del Sistema (usado como referencia)\n"
                  << "   Esperando solicitudes de clientes...\n"
                  << "========================================================\n" << std::endl;

        while (running) {
            Packet req{};
            sockaddr_in client_addr{};
            socklen_val_t client_addr_len = sizeof(client_addr);

            // Esperar una petición del cliente
            int bytes_received = recvfrom(server_sock, reinterpret_cast<char*>(&req), sizeof(req), 0,
                                          reinterpret_cast<sockaddr*>(&client_addr), &client_addr_len);

            if (bytes_received <= 0) {
                if (!running) break;
                continue;
            }

            // Registrar el momento exacto en que llega la petición (según reloj del servidor)
            int64_t t_recv = get_system_time_ms();

            // Validar que sea un paquete válido del protocolo
            if (req.magic != PROTOCOL_MAGIC || req.type != MSG_REQUEST) {
                std::cout << "[WARN] Paquete inválido recibido desde cliente." << std::endl;
                continue;
            }

            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
            uint16_t client_port = ntohs(client_addr.sin_port);

            // Simulamos un pequeño tiempo de procesamiento interno (opcional, pero ayuda a ver el cálculo de I)
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            // Registrar el momento en que enviamos la respuesta
            int64_t t_send = get_system_time_ms();

            // Preparar respuesta con los tiempos del servidor
            Packet resp{};
            resp.magic = PROTOCOL_MAGIC;
            resp.type = MSG_RESPONSE;
            resp.t0_client_send = req.t0_client_send; // devolvemos T0 tal como lo envió el cliente
            resp.t_server_recv  = t_recv;
            resp.t_server_send  = t_send;
            resp.t1_client_recv = 0; // el cliente lo completará cuando reciba la respuesta

            sendto(server_sock, reinterpret_cast<const char*>(&resp), sizeof(resp), 0,
                   reinterpret_cast<sockaddr*>(&client_addr), client_addr_len);

            std::cout << "[SERVIDOR] Solicitud atendida para " << client_ip << ":" << client_port << "\n"
                      << "  -> T_recv servidor: " << format_time_ms(t_recv) << " (" << t_recv << " ms)\n"
                      << "  -> T_send servidor: " << format_time_ms(t_send) << " (" << t_send << " ms)\n"
                      << "  -> Tiempo interno:   " << (t_send - t_recv) << " ms\n" << std::endl;
        }

        close_socket(server_sock);
        socket_cleanup();
    }
};

} // namespace cristian
