#pragma once

// Archivo común del Ejercicio C: Método de Cristian
// Aquí definimos el protocolo UDP, el reloj local simulado y la fórmula para calcular el offset.

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <vector>
#include <thread>
#include <random>

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    using socket_t = SOCKET;
    using socklen_val_t = int;
    constexpr socket_t INVALID_SOCKET_FD = INVALID_SOCKET;
    constexpr int SOCKET_ERROR_VAL = SOCKET_ERROR;
    inline void close_socket(socket_t s) { closesocket(s); }
    inline void socket_cleanup() { WSACleanup(); }
    inline bool socket_init() {
        WSADATA wsaData;
        return (WSAStartup(MAKEWORD(2, 2), &wsaData) == 0);
    }
#else
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <netdb.h>
    using socket_t = int;
    using socklen_val_t = socklen_t;
    constexpr socket_t INVALID_SOCKET_FD = -1;
    constexpr int SOCKET_ERROR_VAL = -1;
    inline void close_socket(socket_t s) { close(s); }
    inline void socket_cleanup() {}
    inline bool socket_init() { return true; }
#endif

namespace cristian {

constexpr uint32_t PROTOCOL_MAGIC = 0x43524953; // "CRIS"
constexpr uint16_t DEFAULT_PORT = 8088;
constexpr uint32_t MSG_REQUEST  = 1;
constexpr uint32_t MSG_RESPONSE = 2;

// Estructura del paquete de sincronización en red (UDP)
// Según el método de Cristian, el cliente necesita conocer los tiempos del servidor
// para estimar el tiempo real descontando el tiempo de ida/vuelta.
#pragma pack(push, 1)
struct Packet {
    uint32_t magic;           // Identificador de protocolo ("CRIS") para validar el paquete
    uint32_t type;            // MSG_REQUEST (petición) o MSG_RESPONSE (respuesta)
    int64_t  t0_client_send;  // T0: Momento en que el cliente envía la petición (medido con su reloj local)
    int64_t  t_server_recv;   // Momento en que el servidor recibe la petición (medido con su reloj)
    int64_t  t_server_send;   // Momento en que el servidor envía la respuesta (medido con su reloj)
    int64_t  t1_client_recv;  // T1: Momento en que el cliente recibe la respuesta (medido con su reloj local)
};
#pragma pack(pop)

// Obtener tiempo actual en milisegundos desde época UNIX
// Usamos el reloj del sistema como "reloj de referencia" (servidor autoritativo).
inline int64_t get_system_time_ms() {
    auto now = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
}

// Formatear milisegundos a hora legible: HH:MM:SS.mmm (más simple para seguir el ejemplo)
inline std::string format_time_ms(int64_t ms) {
    auto sec = std::chrono::seconds(ms / 1000);
    auto remainder_ms = std::abs(ms % 1000);
    std::time_t tt = static_cast<std::time_t>(sec.count());
    std::tm tm_buf{};
#ifdef _WIN32
    localtime_s(&tm_buf, &tt);
#else
    localtime_r(&tt, &tm_buf);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%H:%M:%S")
        << "." << std::setfill('0') << std::setw(3) << remainder_ms;
    return oss.str();
}

// Representa el reloj local de un proceso (con offset y deriva simulables)
// Esto nos permite simular un cliente con reloj atrasado o adelantado sin tocar el reloj real.
class LocalClock {
private:
    int64_t offset_ms;     // Desfase acumulado (ms) entre este reloj y el sistema
    double drift_rate;     // Deriva simulada (ej: +0.001 = avanza 0.1% más rápido que el sistema)
    int64_t base_sys_ms;   // Momento de referencia tomado del reloj del sistema
    int64_t base_clock_ms; // Momento equivalente en este reloj local

public:
    LocalClock(int64_t initial_offset_ms = 0, double drift = 0.0)
        : offset_ms(initial_offset_ms), drift_rate(drift) {
        base_sys_ms = get_system_time_ms();
        base_clock_ms = base_sys_ms + offset_ms; // Partimos con un desfase inicial
    }

    // Devuelve el tiempo actual según este reloj local
    // Avanza proporcional al tiempo que pasa en el sistema, pero con posible deriva
    int64_t now_ms() const {
        int64_t elapsed_sys = get_system_time_ms() - base_sys_ms;
        // Aplicamos deriva si existe (opcional, útil para ver efectos a lo largo del tiempo)
        int64_t elapsed_with_drift = static_cast<int64_t>(elapsed_sys * (1.0 + drift_rate));
        return base_clock_ms + elapsed_with_drift;
    }

    // Ajuste directo (salto / step): suma/resta el offset de golpe
    // Simple, pero si offset < 0 significa "adelantar" el reloj -> puede ir hacia atrás si aplicamos mal
    void step_adjust(int64_t adjustment_ms) {
        base_clock_ms += adjustment_ms;
        offset_ms += adjustment_ms;
    }

    // Ajuste gradual (amortización / slewing): en lugar de saltar, modifica la frecuencia
    // Así evitamos que el reloj retroceda (importante en muchos sistemas)
    void slew_adjust(int64_t adjustment_ms, int duration_seconds = 5) {
        std::cout << "  [Slewing] Iniciando ajuste gradual de " << adjustment_ms 
                  << " ms a lo largo de " << duration_seconds << " segundos (modificando frecuencia)..." << std::endl;
        
        int steps = duration_seconds * 10; // cada 100ms
        int64_t delta_per_step = adjustment_ms / steps;
        int64_t remainder = adjustment_ms % steps;

        for (int i = 0; i < steps; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            int64_t chunk = delta_per_step + (i == steps - 1 ? remainder : 0);
            base_clock_ms += chunk;
            offset_ms += chunk;
        }
        std::cout << "  [Slewing] Ajuste gradual completado sin violar la monotonicidad del reloj." << std::endl;
    }

    int64_t get_current_offset() const {
        return now_ms() - get_system_time_ms();
    }
};

// Resultado del cálculo del Algoritmo de Cristian
struct SyncResult {
    int64_t t0;             // T0: Envío del cliente
    int64_t t_server_recv;  // Recepción en el servidor
    int64_t t_server_send;  // Respuesta enviada por el servidor
    int64_t t1;             // T1: Recepción de la respuesta en el cliente
    int64_t rtt_total;      // RTT total medido por el cliente = T1 - T0
    int64_t proc_time;      // Tiempo que el servidor tardó en procesar = T_send - T_recv
    int64_t rtt_net;        // RTT de red = RTT_total - tiempo de procesamiento
    int64_t one_way_delay;  // Latencia estimada en un sentido (ida o vuelta) = RTT_net / 2 (supone simetría)
    int64_t t_new;          // Tiempo estimado correcto = T_server_send + (RTT_net / 2)
    int64_t offset;         // Desfase a corregir = T_nuevo - T1 (positivo = reloj atrasado, necesita avanzar)
    int64_t t_min;          // Estimado mínimo de tiempo de propagación en un sentido
    double  error_bound;    // Cota de error teórica = ±((RTT_net/2) - T_min)
};

// Función pura de cálculo según el Algoritmo de Cristian
// Pasos:
// 1. Medir RTT total: (T1 - T0)
// 2. Restar tiempo de procesamiento del servidor: RTT_net = RTT_total - (T_send - T_recv)
// 3. Asumir red simétrica: tiempo ida ≈ vuelta ≈ RTT_net/2
// 4. Estimar hora correcta al recibir: T_nuevo = T_server_send + RTT_net/2
// 5. Calcular offset = T_nuevo - T1 (lo que hay que sumar/restar al reloj del cliente)
inline SyncResult calculate_cristian(int64_t t0, int64_t t_server_recv, int64_t t_server_send, int64_t t1, int64_t t_min = 0) {
    SyncResult res{};
    res.t0 = t0;
    res.t_server_recv = t_server_recv;
    res.t_server_send = t_server_send;
    res.t1 = t1;

    // 1) RTT medido por el cliente
    res.rtt_total = t1 - t0;
    // 2) Descontar tiempo que tardó el servidor en responder
    res.proc_time = std::max<int64_t>(0, t_server_send - t_server_recv);
    res.rtt_net = std::max<int64_t>(0, res.rtt_total - res.proc_time);
    
    // 3) Suponemos ida y vuelta con la misma latencia
    res.one_way_delay = res.rtt_net / 2;
    // 4) Estimamos la hora correcta al momento en que el cliente recibió la respuesta
    res.t_new = res.t_server_send + res.one_way_delay;
    // 5) Lo que debemos ajustar: si es positivo, el reloj del cliente está atrasado -> hay que sumarle
    res.offset = res.t_new - res.t1;
    res.t_min = t_min;

    // Cota de error teórica según Cristian: el tiempo real puede estar entre
    // T_send + T_min y T_send + (RTT_net - T_min), por eso el error es ±((RTT_net/2) - T_min)
    res.error_bound = static_cast<double>(res.rtt_net) / 2.0 - static_cast<double>(t_min);
    if (res.error_bound < 0) res.error_bound = 0;

    return res;
}

} // namespace cristian
