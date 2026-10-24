#include "cristian_common.hpp"
#include "cristian_server.hpp"
#include "cristian_client.hpp"
#include "cristian_simulation.hpp"
#include <iostream>
#include <string>

void show_help(const char* prog) {
    std::cout << "========================================================\n"
              << "  MÉTODO DE CRISTIAN - SINCRONIZACIÓN DE RELOJES EN C++\n"
              << "  Cátedra: Sistemas Operativos - UADER FCyT\n"
              << "========================================================\n"
              << "Uso:\n"
              << "  " << prog << " demo                       Ejecuta la simulación y análisis teórico completo.\n"
              << "  " << prog << " server [puerto]            Inicia el servidor de tiempo UDP (default: 8088).\n"
              << "  " << prog << " client [ip] [puerto]       Ejecuta un cliente y sincroniza su reloj local.\n"
              << "  " << prog << " multi  [ip] [puerto] [N]   Ejecuta cliente con filtro de N muestras (min RTT).\n"
              << "  " << prog << "                            Modo interactivo (menú en consola).\n"
              << "========================================================\n";
}

void interactive_menu() {
    while (true) {
        std::cout << "\n========================================================" << std::endl;
        std::cout << "   EJERCICIO C: MÉTODO DE CRISTIAN (UADER - SO)" << std::endl;
        std::cout << "========================================================" << std::endl;
        std::cout << "1. Ejecutar Simulación y Análisis Teórico Completo (Demo)" << std::endl;
        std::cout << "2. Iniciar Servidor de Tiempo Cristian (UDP)" << std::endl;
        std::cout << "3. Iniciar Cliente Cristian (Sincronización Simple)" << std::endl;
        std::cout << "4. Iniciar Cliente Cristian con Filtro de Múltiples Muestras" << std::endl;
        std::cout << "5. Salir" << std::endl;
        std::cout << "Seleccione una opción [1-5]: ";

        int opc = 0;
        if (!(std::cin >> opc)) {
            std::cin.clear();
            std::string discard;
            std::cin >> discard;
            continue;
        }

        if (opc == 1) {
            cristian::CristianSimulation::run_full_demo();
        } else if (opc == 2) {
            uint16_t port = cristian::DEFAULT_PORT;
            std::cout << "Puerto UDP [" << port << "]: ";
            std::string input;
            std::cin.ignore();
            std::getline(std::cin, input);
            if (!input.empty()) {
                port = static_cast<uint16_t>(std::stoi(input));
            }
            cristian::CristianServer server(port);
            server.run();
        } else if (opc == 3) {
            std::string ip = "127.0.0.1";
            uint16_t port = cristian::DEFAULT_PORT;
            int64_t drift = 4200;

            std::cout << "IP del Servidor [" << ip << "]: ";
            std::string input;
            std::cin.ignore();
            std::getline(std::cin, input);
            if (!input.empty()) ip = input;

            std::cout << "Puerto del Servidor [" << port << "]: ";
            std::getline(std::cin, input);
            if (!input.empty()) port = static_cast<uint16_t>(std::stoi(input));

            std::cout << "Desfase artificial inicial del cliente en ms (ej: -3000 o +4500) [" << drift << "]: ";
            std::getline(std::cin, input);
            if (!input.empty()) drift = std::stoll(input);

            std::cout << "¿Usar slewing si el reloj está adelantado? (s/N): ";
            std::getline(std::cin, input);
            bool slew = (!input.empty() && (input[0] == 's' || input[0] == 'S'));

            cristian::CristianClient client(ip, port, drift);
            client.sync_once(true, slew);
        } else if (opc == 4) {
            std::string ip = "127.0.0.1";
            uint16_t port = cristian::DEFAULT_PORT;
            int samples = 5;

            std::cout << "IP del Servidor [" << ip << "]: ";
            std::string input;
            std::cin.ignore();
            std::getline(std::cin, input);
            if (!input.empty()) ip = input;

            std::cout << "Puerto del Servidor [" << port << "]: ";
            std::getline(std::cin, input);
            if (!input.empty()) port = static_cast<uint16_t>(std::stoi(input));

            std::cout << "Cantidad de muestras [" << samples << "]: ";
            std::getline(std::cin, input);
            if (!input.empty()) samples = std::stoi(input);

            cristian::CristianClient client(ip, port, -3500);
            client.sync_best_of_n(samples);
        } else if (opc == 5) {
            std::cout << "Saliendo..." << std::endl;
            break;
        }
    }
}

int main(int argc, char* argv[]) {
    // Configurar consola Windows para UTF-8 si corresponde
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    if (argc == 1) {
        interactive_menu();
        return 0;
    }

    std::string cmd = argv[1];
    if (cmd == "demo" || cmd == "--demo" || cmd == "-d") {
        cristian::CristianSimulation::run_full_demo();
    } else if (cmd == "server") {
        uint16_t port = (argc >= 3) ? static_cast<uint16_t>(std::stoi(argv[2])) : cristian::DEFAULT_PORT;
        cristian::CristianServer server(port);
        server.run();
    } else if (cmd == "client") {
        std::string ip = (argc >= 3) ? argv[2] : "127.0.0.1";
        uint16_t port = (argc >= 4) ? static_cast<uint16_t>(std::stoi(argv[3])) : cristian::DEFAULT_PORT;
        cristian::CristianClient client(ip, port, 3500);
        client.sync_once(true, false);
    } else if (cmd == "multi") {
        std::string ip = (argc >= 3) ? argv[2] : "127.0.0.1";
        uint16_t port = (argc >= 4) ? static_cast<uint16_t>(std::stoi(argv[3])) : cristian::DEFAULT_PORT;
        int samples = (argc >= 5) ? std::stoi(argv[4]) : 5;
        cristian::CristianClient client(ip, port, -5000);
        client.sync_best_of_n(samples);
    } else {
        show_help(argv[0]);
    }

    return 0;
}
