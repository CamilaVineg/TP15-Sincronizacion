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
- **Ejercicio C — Método de Cristian:** Implementación cliente-servidor para sincronización de reloj con compensación del tiempo de ida y vuelta (RTT).

---

## 🚀 Requisitos y Compilación

Para compilar y ejecutar los programas en C++:

- Compilador compatible con **C++17** o superior (`g++` / `clang` / `MSVC`).
- Herramienta de construcción opcional: `make` o `CMake`.

```bash
# Ejemplo de compilación con g++
g++ -std=c++17 -Wall -Wextra src/ejercicio_a_lamport.cpp -o bin/lamport
./bin/lamport
```
