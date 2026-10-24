# TP 15 - Sincronización en Sistemas Distribuidos

Trabajo Práctico N° 15 de la cátedra de **Sistemas Operativos** — **FCyT (Sede Concepción del Uruguay) - UADER**.

---

## 👥 Integrantes
- **Pérez, Sofía**
- **Viñeg, Camila**
- **Poggio, Abril**
- **Rochás, Tadeo**

### 👨‍🏫 Docentes
- **Ing. Osvaldo Aguiar**
- **Mg. Ing. Ulises Rapallini**

**Año Académico:** 2026  

---

## 📖 Descripción del Trabajo

Este trabajo práctico aborda los conceptos y mecanismos fundamentales para la **sincronización de procesos y relojes en Sistemas Distribuidos**, tanto a nivel teórico como mediante implementaciones prácticas en **C++**.

---

## 📋 Temario y Contenido

### 1. Cuestionario Teórico
- **A. Reloj Físico vs. Reloj Lógico:** Diferencias conceptuales y casos de uso.
- **B. Tiempo Atómico Internacional (TAI / UTC):** Definición, origen y estándares de tiempo.
- **C. Deriva de Reloj (Clock Drift):** Causas físicas, tasa de deriva y consecuencias en sistemas distribuidos.
- **D. Observaciones de Lamport:** La noción de causalidad, la relación *happens-before* ($\to$) y la relativización del tiempo físico.
- **E. Algoritmo de Lamport:** Funcionamiento paso a paso, estampas de tiempo lógicas y ejemplo práctico.
- **F. Algoritmos Centralizados vs. Distribuidos:** Comparativa de arquitecturas de sincronización.
- **G. Métodos de Arbitraje:** Análisis de ventajas y desventajas entre esquemas centralizados y distribuidos.
- **H. Algoritmo del Matón (Bully Algorithm):** Elección de coordinador en entornos distribuidos tolerantes a fallos.
- **I. Estados, Eventos y Relojes en un S.D.:** Interrelación entre los cambios de estado local y el avance temporal.
- **J. Network Time Protocol (NTP):** Arquitectura jerárquica (estratos), intercambio de paquetes y cálculo de offset/dispersión.
- **K. Estado Global en un S.D.:** Cortes consistentes, vistas coherentes y algoritmos de captura (Chandy-Lamport).

---

### 2. Ejercicios Prácticos (C++)
- **Ejercicio A — Algoritmo de Lamport:** Implementación de un sistema distribuido con 4 procesos ($P_1, P_2, P_3, P_4$) sincronizados para garantizar la secuencia de ejecución ordenada $P_1 \to P_2 \to P_3 \to P_4$.
- **Ejercicio B — Algoritmo de Berkeley:** Simulación de sincronización interna de relojes mediante un maestro activo que recolecta tiempos, calcula el promedio y distribuye los ajustes relativos a los esclavos.
- **Ejercicio C — Método de Cristian (Implementado):** 
  - Implementación cliente-servidor mediante sockets UDP y simulación didáctica integral.
  - Medición de $T_0$, $T_{\text{recv}}$, $T_{\text{send}}$ y $T_1$.
  - Descuento del tiempo de procesamiento interno del servidor ($I$).
  - Cálculo del tiempo estimado $T_{\text{nuevo}} = T_{\text{send}} + \frac{RTT_{\text{net}}}{2}$.
  - Cálculo del desfase local $\theta = T_{\text{nuevo}} - T_1$ y cota de error $\pm \left( \frac{RTT_{\text{net}}}{2} - T_{\min} \right)$.
  - Soporte de ajuste por salto (*step*) y ajuste gradual por amortización de frecuencia (*slewing*) para garantizar monotonicidad (no retroceder el tiempo).
  - Filtro de Cristian para muestreo múltiple: realiza $N$ solicitudes y selecciona la muestra de menor RTT para minimizar la incertidumbre.

---

## 🛠️ Estructura del Proyecto

```text
TP 15 -Sincronización/
├── CMakeLists.txt              # Configuración de compilación multiplataforma (CMake)
├── build.bat                   # Script automático de compilación para Windows
├── .gitignore                  # Exclusión de binarios y temporales
├── README.md                   # Documentación general y teórica
└── src/
    └── ejercicio_c_cristian/
        ├── cristian_common.hpp     # Protocolo UDP, estructura de paquetes, reloj local y fórmulas
        ├── cristian_server.hpp     # Servidor de tiempo autoritativo UDP
        ├── cristian_client.hpp     # Cliente UDP con ajuste por salto, amortización (slewing) y multi-muestreo
        ├── cristian_simulation.hpp # Demostración didáctica y simulador de enlaces de red
        └── main.cpp                # CLI interactiva y despachador de comandos
```

---

## 🚀 Compilación y Ejecución (Ejercicio C)

### Compilación

**Opción 1: Con CMake (Recomendado / Multiplataforma)**
```bash
cmake -B build
cmake --build build
```

**Opción 2: Con g++ (MinGW / GCC)**
```bash
g++ -std=c++17 -Wall -Wextra src/ejercicio_c_cristian/main.cpp -o bin/cristian.exe -lws2_32
```

**Opción 3: Script automático en Windows**
```cmd
build.bat
```

---

### Modos de Ejecución

El programa ofrece varios modos de funcionamiento:

1. **Modo Interactivo (Menú en Consola):**
   ```bash
   ./bin/cristian
   ```
   Despliega un menú con opciones para ejecutar la demostración teórica guiada, iniciar el servidor UDP o conectarse como cliente.

2. **Modo Demostración / Simulación Guiada:**
   ```bash
   ./bin/cristian demo
   ```
   Ejecuta todos los casos de estudio teóricos:
   - Cliente con reloj atrasado y compensación de RTT.
   - Cliente con reloj adelantado y preservación de monotonicidad vía *slewing*.
   - Análisis de sensibilidad y desvío ante retardos de red asimétricos.
   - Demostración del filtro de Cristian eligiendo la muestra con menor RTT para contrarrestar *jitter*.

3. **Modo Red Real (Servidor UDP):**
   ```bash
   ./bin/cristian server 8088
   ```

4. **Modo Red Real (Cliente UDP Simple):**
   ```bash
   ./bin/cristian client 127.0.0.1 8088
   ```

5. **Modo Red Real (Cliente con Filtro de Múltiples Muestras):**
   ```bash
   ./bin/cristian multi 127.0.0.1 8088 5
   ```

