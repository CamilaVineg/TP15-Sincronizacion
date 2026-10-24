#pragma once

#include "cristian_common.hpp"

namespace cristian {

class CristianSimulation {
public:
    static void run_full_demo() {
        std::cout << "\n======================================================================\n"
                  << "  DEMOSTRACIÓN INTERACTIVA: ALGORITMO DE SINCRONIZACIÓN DE CRISTIAN\n"
                  << "  Sistemas Operativos - UADER FCyT\n"
                  << "======================================================================\n" << std::endl;

        std::cout << "1. FUNDAMENTO TEÓRICO:\n"
                  << "   - Modelo Cliente-Servidor de sincronización externa.\n"
                  << "   - El servidor posee un reloj de referencia confiable (UTC / Reloj Físico).\n"
                  << "   - El cliente envía una petición en T0 y recibe la respuesta en T1.\n"
                  << "   - RTT = (T1 - T0) - Tiempo_Procesamiento.\n"
                  << "   - El cliente estima el tiempo actual como: T_nuevo = T_servidor + (RTT / 2).\n"
                  << "   - Ajuste necesario: Offset = T_nuevo - T1.\n"
                  << "   - Precisión alcanzada: +-((RTT / 2) - T_min).\n\n";

        // Caso 1: Sincronización básica con compensación de RTT
        std::cout << "----------------------------------------------------------------------\n";
        std::cout << "CASO 1: CLIENTE CON RELOJ ATRASADO (-5000 ms) Y LATENCIA SIMÉTRICA\n";
        std::cout << "----------------------------------------------------------------------\n";
        simulate_node("Cliente_Alfa", -5000, 25, 25, 3, 5);

        // Caso 2: Cliente adelantado con ajuste gradual (slewing)
        std::cout << "\n----------------------------------------------------------------------\n";
        std::cout << "CASO 2: CLIENTE CON RELOJ ADELANTADO (+3500 ms) Y AMORTIZACIÓN (SLEWING)\n";
        std::cout << "----------------------------------------------------------------------\n";
        std::cout << "  (Principio de monotonicidad: el tiempo nunca debe retroceder)\n";
        simulate_node("Cliente_Beta", 3500, 40, 40, 2, 8, true);

        // Caso 3: Efecto de la asimetría en la red
        std::cout << "\n----------------------------------------------------------------------\n";
        std::cout << "CASO 3: ANÁLISIS DE SENSIBILIDAD ANTE LATENCIAS ASIMÉTRICAS\n";
        std::cout << "----------------------------------------------------------------------\n";
        std::cout << "  (Cristian asume latencia ida = latencia vuelta. ¿Qué ocurre si no lo es?)\n";
        simulate_asymmetric_analysis();

        // Caso 4: Filtro de selección por menor RTT (Muestreo múltiple)
        std::cout << "\n----------------------------------------------------------------------\n";
        std::cout << "CASO 4: TÉCNICA DE MÚLTIPLES MUESTRAS PARA REDUCIR ERROR POR COLA/JITTER\n";
        std::cout << "----------------------------------------------------------------------\n";
        simulate_multi_sample_filter();

        std::cout << "\n======================================================================\n";
        std::cout << "  FIN DE LA DEMOSTRACIÓN DEL ALGORITMO DE CRISTIAN\n";
        std::cout << "======================================================================\n" << std::endl;
    }

private:
    static void simulate_node(const std::string& name, int64_t initial_offset, 
                              int64_t forward_delay_ms, int64_t backward_delay_ms, 
                              int64_t server_proc_ms, int64_t t_min, bool demo_slew = false) {
        LocalClock client_clock(initial_offset);
        int64_t ref_time_now = get_system_time_ms();

        std::cout << "  [>] Nodo: " << name << "\n";
        std::cout << "      Reloj Real (Servidor): " << format_time_ms(ref_time_now) << " (" << ref_time_now << " ms)\n";
        std::cout << "      Reloj Local (Cliente):  " << format_time_ms(client_clock.now_ms()) 
                  << " (Desfase inicial: " << initial_offset << " ms)\n";

        // T0: Momento en que el cliente despacha la petición
        int64_t t0 = client_clock.now_ms();
        int64_t real_at_t0 = get_system_time_ms();

        // Tránsito de red (Ida)
        int64_t real_at_server_recv = real_at_t0 + forward_delay_ms;
        int64_t t_server_recv = real_at_server_recv; // El servidor tiene el reloj real

        // Procesamiento en el servidor
        int64_t real_at_server_send = real_at_server_recv + server_proc_ms;
        int64_t t_server_send = real_at_server_send;

        // Tránsito de red (Vuelta)
        int64_t real_at_t1 = real_at_server_send + backward_delay_ms;
        
        // El reloj del cliente avanza durante el tiempo transcurrido (RTT físico)
        int64_t elapsed_physical = real_at_t1 - real_at_t0;
        int64_t t1 = t0 + elapsed_physical;

        SyncResult sr = calculate_cristian(t0, t_server_recv, t_server_send, t1, t_min);

        std::cout << "\n      Tiempos registrados:\n";
        std::cout << "      - T0 (Cliente envía):      " << format_time_ms(t0) << " (" << t0 << " ms)\n";
        std::cout << "      - T_recv (Servidor entra): " << format_time_ms(t_server_recv) << " (" << t_server_recv << " ms)\n";
        std::cout << "      - T_send (Servidor sale):  " << format_time_ms(t_server_send) << " (" << t_server_send << " ms)\n";
        std::cout << "      - T1 (Cliente recibe):     " << format_time_ms(t1) << " (" << t1 << " ms)\n";
        std::cout << "      - RTT Total Medido:        " << sr.rtt_total << " ms (Neto de red: " << sr.rtt_net << " ms)\n";
        std::cout << "      - Latencia 1 vía estimada: " << sr.one_way_delay << " ms\n";
        std::cout << "      - T_nuevo calculado:       " << format_time_ms(sr.t_new) << " (" << sr.t_new << " ms)\n";
        std::cout << "      - Desfase (Offset):        " << sr.offset << " ms\n";
        std::cout << "      - Cota teórica de error:   +-" << std::fixed << std::setprecision(1) << sr.error_bound << " ms\n";

        if (demo_slew && sr.offset < 0) {
            std::cout << "\n      [Aplicando Corrección por Slewing]:\n";
            std::cout << "      Como el offset es negativo (" << sr.offset << " ms), se ralentiza el reloj para no volver atrás en el tiempo.\n";
            client_clock.slew_adjust(sr.offset, 2);
        } else {
            client_clock.step_adjust(sr.offset);
        }

        int64_t real_after = get_system_time_ms();
        std::cout << "      -> Estado Final del Reloj Cliente: " << format_time_ms(client_clock.now_ms()) << "\n";
        std::cout << "      -> Error residual real:            " << (client_clock.now_ms() - real_after) << " ms\n";
    }

    static void simulate_asymmetric_analysis() {
        int64_t rtt_net = 60; // 60 ms totales de red
        // Supongamos que la ida tardó 10ms y la vuelta 50ms (por ruteo asimétrico o cola de descarga)
        int64_t forward_delay = 10;
        int64_t backward_delay = 50;

        int64_t t_server_send = 100000;
        // Cristian asume simetría: estima que la vuelta tardó RTT/2 = 30 ms
        int64_t d_est = rtt_net / 2; // 30 ms
        int64_t t_new = t_server_send + d_est; // 100030

        // El tiempo real al momento de recepción fue:
        int64_t t_real_arrival = t_server_send + backward_delay; // 100050

        int64_t error_asymmetry = t_new - t_real_arrival; // -20 ms

        std::cout << "  Escenario: RTT neto = 60 ms.\n"
                  << "  - Latencia real de IDA:    " << forward_delay << " ms\n"
                  << "  - Latencia real de VUELTA: " << backward_delay << " ms\n"
                  << "  - Estimación de Cristian (RTT/2): " << d_est << " ms\n"
                  << "  - Tiempo real de llegada:         " << t_real_arrival << " ms\n"
                  << "  - Tiempo calculado por Cristian:  " << t_new << " ms\n"
                  << "  - Error inducido por asimetría:   " << error_asymmetry << " ms\n"
                  << "  Conclusión: La precisión del algoritmo de Cristian depende de la simetría de la red.\n";
    }

    static void simulate_multi_sample_filter() {
        std::cout << "  Simulando 5 solicitudes sucesivas con congestión variable en el canal:\n\n";
        struct SampleSim {
            int id;
            int64_t ida;
            int64_t vuelta;
            int64_t proc;
            int64_t t_min;
        };

        std::vector<SampleSim> samples = {
            {1, 45, 60, 2, 5}, // Mucha cola
            {2, 12, 15, 2, 5}, // Canal libre (Excelente)
            {3, 80, 95, 3, 5}, // Congestión severa
            {4, 20, 22, 2, 5}, // Aceptable
            {5, 35, 40, 2, 5}  // Moderada
        };

        int best_id = -1;
        int64_t min_rtt = 999999;
        double best_error_bound = 999999;

        for (const auto& s : samples) {
            int64_t rtt_total = s.ida + s.vuelta + s.proc;
            int64_t rtt_net = s.ida + s.vuelta;
            double error_bound = (static_cast<double>(rtt_net) / 2.0) - static_cast<double>(s.t_min);

            std::cout << "  [Muestra #" << s.id << "] RTT Neto = " << std::setw(3) << rtt_net 
                      << " ms (Ida: " << s.ida << "ms, Vuelta: " << s.vuelta << "ms) "
                      << "| Incertidumbre = +-" << std::fixed << std::setprecision(1) << error_bound << " ms";

            if (rtt_net < min_rtt) {
                min_rtt = rtt_net;
                best_id = s.id;
                best_error_bound = error_bound;
                std::cout << "  <-- NUEVO MÍNIMO (Muestra elegida)";
            }
            std::cout << "\n";
        }

        std::cout << "\n  -> Decisión del algoritmo de Cristian: Se adopta la MUESTRA #" << best_id 
                  << " con RTT neto de " << min_rtt << " ms.\n"
                  << "  -> La incertidumbre se redujo de +-85.0 ms (en la peor muestra) a solo +-" 
                  << best_error_bound << " ms.\n";
    }
};

} // namespace cristian
