/**
 * @file main.cpp
 * @brief Sistema de Gestión Polimórfica de Sensores IoT con lista enlazada genérica (sin STL).
 * @details
 *  - ListaSensor<T>: lista enlazada simple genérica con Regla de los Tres (destructor, copia, asignación).
 *  - Jerarquía polimórfica: SensorBase (abstracta), SensorTemperatura(float), SensorPresion(int).
 *  - Lista de gestión polimórfica (no genérica) que guarda SensorBase* y libera en cascada.
 *  - Menú de consola para crear sensores, registrar lecturas, y ejecutar procesamiento polimórfico.
 *  - Opcional: ingestión de líneas estilo "ID,valor" para simular Serial/Arduino.
 *
 * @author
 *   Equipo IC – ITIID
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>

/* ============================================================
 *           Lista enlazada genérica (sin STL)
 * ============================================================*/

/**
 * @brief Lista enlazada simple genérica sin STL.
 * @tparam T Tipo de dato almacenado (int, float, double, etc.)
 *
 * Regla de los Tres implementada:
 *  - Destructor ~ListaSensor()
 *  - Constructor de copia ListaSensor(const ListaSensor&)
 *  - Operador de asignación ListaSensor& operator=(const ListaSensor&)
 *
 * Operaciones:
 *  - push_back
 *  - size
 *  - sum (para métricas sencillas)
 *  - pop_min (elimina el mínimo, útil p/temperatura)
 *  - find_first (búsqueda simple por igualdad)
 *  - clear
 */
template <typename T>
class ListaSensor {
public:
    struct Nodo {
        T dato;
        Nodo* siguiente;
        Nodo(const T& v) : dato(v), siguiente(NULL) {}
    };

private:
    Nodo* cabeza;
    Nodo* cola;
    size_t n;

    void copiarDesde(const ListaSensor& other) {
        Nodo* it = other.cabeza;
        while (it) {
            push_back(it->dato);
            it = it->siguiente;
        }
    }

public:
    ListaSensor() : cabeza(NULL), cola(NULL), n(0) {}

    // Constructor de copia
    ListaSensor(const ListaSensor& other) : cabeza(NULL), cola(NULL), n(0) {
        copiarDesde(other);
    }

    // Operador asignación
    ListaSensor& operator=(const ListaSensor& other) {
        if (this != &other) {
            clear();
            copiarDesde(other);
        }
        return *this;
    }

    ~ListaSensor() {
        clear();
    }

    void push_back(const T& v) {
        Nodo* nuevo = new Nodo(v);
        if (!cabeza) {
            cabeza = cola = nuevo;
        } else {
            cola->siguiente = nuevo;
            cola = nuevo;
        }
        n++;
    }

    size_t size() const { return n; }

    /**
     * @brief Suma de elementos (solo para tipos numéricos).
     * @return Suma de todos los valores (0 si lista vacía).
     */
    T sum() const {
        if (!cabeza) return T(0);
        T s = T(0);
        Nodo* it = cabeza;
        while (it) {
            s += it->dato;
            it = it->siguiente;
        }
        return s;
    }

    /**
     * @brief Elimina el valor mínimo. Devuelve true si eliminó alguno.
     */
    bool pop_min(T& outMin) {
        if (!cabeza) return false;

        Nodo* minPrev = NULL;
        Nodo* minNode = cabeza;

        Nodo* prev = NULL;
        Nodo* curr = cabeza;
        while (curr) {
            if (curr->dato < minNode->dato) {
                minNode = curr;
                minPrev = prev;
            }
            prev = curr;
            curr = curr->siguiente;
        }

        outMin = minNode->dato;

        if (minPrev == NULL) { // min es cabeza
            cabeza = minNode->siguiente;
            if (cola == minNode) cola = cabeza;
        } else {
            minPrev->siguiente = minNode->siguiente;
            if (cola == minNode) cola = minPrev;
        }

        // Log de liberación
        printf("    [Log] Nodo liberado con valor: %s\n",
               formatNumber(outMin));
        delete minNode;
        n--;
        return true;
    }

    /**
     * @brief Encuentra la primera coincidencia exacta (==).
     */
    bool find_first(const T& value, T& found) const {
        Nodo* it = cabeza;
        while (it) {
            if (it->dato == value) {
                found = it->dato;
                return true;
            }
            it = it->siguiente;
        }
        return false;
    }

    void clear() {
        Nodo* it = cabeza;
        while (it) {
            Nodo* nxt = it->siguiente;
            delete it;
            it = nxt;
        }
        cabeza = cola = NULL;
        n = 0;
    }

    // Utilidad de impresión (debug)
    void print_all(const char* prefix = "") const {
        printf("%s[", prefix);
        Nodo* it = cabeza;
        while (it) {
            printf("%s", formatNumber(it->dato));
            if (it->siguiente) printf(", ");
            it = it->siguiente;
        }
        printf("]\n");
    }

private:
    // Helpers sin STL: formateo de números a C-string temporal
    static const char* formatNumber(int v) {
        static char buf[64];
        std::snprintf(buf, sizeof(buf), "%d", v);
        return buf;
    }
    static const char* formatNumber(float v) {
        static char buf[64];
        std::snprintf(buf, sizeof(buf), "%.3f", v);
        return buf;
    }
    static const char* formatNumber(double v) {
        static char buf[64];
        std::snprintf(buf, sizeof(buf), "%.6f", v);
        return buf;
    }
};

/* ============================================================
 *            Jerarquía polimórfica de sensores
 * ============================================================*/

/**
 * @brief Clase base abstracta de sensores.
 */
class SensorBase {
protected:
    char nombre[50]; ///< Identificador del sensor (e.g., "T-001")

public:
    SensorBase(const char* id = "UNNAMED") {
        std::strncpy(nombre, id, sizeof(nombre));
        nombre[sizeof(nombre) - 1] = '\0';
    }

    virtual ~SensorBase() {} // VIRTUAL para liberar derivadas

    const char* getNombre() const { return nombre; }

    /**
     * @brief Procesa las lecturas internas de cada sensor (polimórfico).
     */
    virtual void procesarLectura() = 0;

    /**
     * @brief Imprime información resumen del sensor.
     */
    virtual void imprimirInfo() const = 0;

    /**
     * @brief Registra una lectura desde texto (general para menú/serial).
     *        Cada derivada la parsea a su tipo.
     */
    virtual bool registrarDesdeTexto(const char* texto) = 0;
};

/**
 * @brief Sensor concreto: Temperatura (usa float).
 *  - Contiene ListaSensor<float> historial
 *  - Procesamiento: Elimina la lectura mínima y muestra promedio restante.
 */
class SensorTemperatura : public SensorBase {
private:
    ListaSensor<float> historial;

public:
    SensorTemperatura(const char* id) : SensorBase(id) {}

    virtual ~SensorTemperatura() {
        printf("  [Destructor Sensor %s] Liberando Lista Interna (float)...\n", nombre);
        // historial.clear() se llama en su destructor automáticamente.
    }

    void agregar(float v) {
        printf("[Log] Insertando Nodo<float> en %s.\n", nombre);
        historial.push_back(v);
    }

    virtual bool registrarDesdeTexto(const char* texto) {
        // Texto esperado: número float, e.g. "45.3"
        if (!texto) return false;
        float v = 0.0f;
        if (std::sscanf(texto, "%f", &v) == 1) {
            agregar(v);
            return true;
        }
        return false;
    }

    virtual void procesarLectura() {
        printf("-> Procesando Sensor %s (Temperatura)...\n", nombre);
        if (historial.size() == 0) {
            printf("[Sensor Temp] No hay lecturas.\n");
            return;
        }

        // Eliminar mínima y reportar promedio del resto
        float eliminado = 0.0f;
        bool ok = historial.pop_min(eliminado);
        if (ok) {
            size_t n = historial.size();
            float promedio = (n > 0) ? (historial.sum() / (float)n) : 0.0f;
            printf("[Sensor Temp] Lectura más baja (%.3f) eliminada. Promedio restante: %.3f.\n",
                   eliminado, promedio);
        } else {
            // Si no pudo eliminar, solo computa promedio
            size_t n = historial.size();
            float promedio = (n > 0) ? (historial.sum() / (float)n) : 0.0f;
            printf("[Sensor Temp] Promedio calculado sobre %zu lectura(s): %.3f.\n", n, promedio);
        }
    }

    virtual void imprimirInfo() const {
        printf("[%s] (Temperatura)\n", nombre);
    }
};

/**
 * @brief Sensor concreto: Presión (usa int).
 *  - Contiene ListaSensor<int> historial
 *  - Procesamiento: Calcula promedio de lecturas (sin eliminar).
 */
class SensorPresion : public SensorBase {
private:
    ListaSensor<int> historial;

public:
    SensorPresion(const char* id) : SensorBase(id) {}

    virtual ~SensorPresion() {
        printf("  [Destructor Sensor %s] Liberando Lista Interna (int)...\n", nombre);
        // historial.clear() en destructor
    }

    void agregar(int v) {
        printf("[Log] Insertando Nodo<int> en %s.\n", nombre);
        historial.push_back(v);
    }

    virtual bool registrarDesdeTexto(const char* texto) {
        // Texto esperado: número entero, e.g. "85"
        if (!texto) return false;
        int v = 0;
        if (std::sscanf(texto, "%d", &v) == 1) {
            agregar(v);
            return true;
        }
        return false;
    }

    virtual void procesarLectura() {
        printf("-> Procesando Sensor %s (Presion)...\n", nombre);
        size_t n = historial.size();
        if (n == 0) {
            printf("[Sensor Presion] No hay lecturas.\n");
            return;
        }
        int s = historial.sum();
        float promedio = (float)s / (float)n;
        printf("[Sensor Presion] Promedio de lecturas: %.3f (sobre %zu lecturas).\n", promedio, n);
    }

    virtual void imprimirInfo() const {
        printf("[%s] (Presion)\n", nombre);
    }
};

/* ============================================================
 *    Lista de gestión polimórfica (SensorBase*)
 * ============================================================*/

class ListaGeneral {
public:
    struct Nodo {
        SensorBase* sensor;
        Nodo* siguiente;
        Nodo(SensorBase* ptr) : sensor(ptr), siguiente(NULL) {}
    };

private:
    Nodo* cabeza;
    Nodo* cola;
    size_t n;

public:
    ListaGeneral() : cabeza(NULL), cola(NULL), n(0) {}

    ~ListaGeneral() {
        liberarTodo();
    }

    void push_back(SensorBase* s) {
        Nodo* nuevo = new Nodo(s);
        if (!cabeza) {
            cabeza = cola = nuevo;
        } else {
            cola->siguiente = nuevo;
            cola = nuevo;
        }
        n++;
    }

    size_t size() const { return n; }

    /**
     * @brief Busca por nombre (id) exacto. Retorna puntero o NULL.
     */
    SensorBase* buscarPorNombre(const char* id) {
        Nodo* it = cabeza;
        while (it) {
            if (std::strcmp(it->sensor->getNombre(), id) == 0) {
                return it->sensor;
            }
            it = it->siguiente;
        }
        return NULL;
    }

    /**
     * @brief Itera y llama procesarLectura() en todos los sensores.
     */
    void procesarTodos() {
        printf("\n--- Ejecutando Polimorfismo ---\n");
        Nodo* it = cabeza;
        while (it) {
            it->sensor->procesarLectura();
            it = it->siguiente;
        }
    }

    /**
     * @brief Libera nodos y, en cascada, cada SensorBase* (virtual dtor).
     */
    void liberarTodo() {
        if (!cabeza) return;
        printf("\n--- Liberación de Memoria en Cascada ---\n");
        Nodo* it = cabeza;
        while (it) {
            Nodo* nxt = it->siguiente;
            printf("[Destructor General] Liberando Nodo: %s.\n", it->sensor->getNombre());
            delete it->sensor;  // destructor virtual -> baja a derivadas
            delete it;
            it = nxt;
        }
        cabeza = cola = NULL;
        n = 0;
        printf("Sistema cerrado. Memoria limpia.\n");
    }

    void imprimirResumen() const {
        printf("\n--- Sensores en la lista (%zu) ---\n", n);
        Nodo* it = cabeza;
        while (it) {
            it->sensor->imprimirInfo();
            it = it->siguiente;
        }
    }
};

/* ============================================================
 *    Utilidades de “Serial” simulado (opcional)
 * ============================================================*/

/**
 * @brief Parsea una línea con formato "ID,valor" y registra en el sensor si existe.
 * @param linea Ej: "T-001,45.3" o "P-105,85"
 * @param lista Referencia a la lista polimórfica
 * @return true si se pudo registrar, false en caso contrario.
 */
bool procesarLineaSerial(const char* linea, ListaGeneral& lista) {
    if (!linea) return false;

    // copias temporales (sin STL)
    char id[64] = {0};
    char valor[64] = {0};

    const char* coma = std::strchr(linea, ',');
    if (!coma) return false;

    size_t lenId = (size_t)(coma - linea);
    if (lenId >= sizeof(id)) lenId = sizeof(id) - 1;
    std::memcpy(id, linea, lenId);
    id[lenId] = '\0';

    const char* pVal = coma + 1;
    std::strncpy(valor, pVal, sizeof(valor) - 1);
    // limpiar salto de línea
    size_t lv = std::strlen(valor);
    if (lv && (valor[lv-1] == '\n' || valor[lv-1] == '\r')) valor[lv-1] = '\0';

    SensorBase* s = lista.buscarPorNombre(id);
    if (!s) {
        printf("[Serial] ID no encontrado: %s\n", id);
        return false;
    }
    bool ok = s->registrarDesdeTexto(valor);
    if (!ok) {
        printf("[Serial] Valor inválido para %s: %s\n", id, valor);
    }
    return ok;
}

/* ============================================================
 *                      Menú principal
 * ============================================================*/

void menu() {
    printf("\n--- Sistema IoT de Monitoreo Polimórfico ---\n");
    printf("1) Crear Sensor de Temperatura (FLOAT)\n");
    printf("2) Crear Sensor de Presion    (INT)\n");
    printf("3) Registrar Lectura manual\n");
    printf("4) Ejecutar Procesamiento Polimorfico\n");
    printf("5) Mostrar sensores\n");
    printf("6) Inyectar linea estilo Serial (ID,valor)\n");
    printf("0) Salir\n");
    printf("Opcion: ");
}

int main() {
    ListaGeneral gestion;

    int opcion = -1;
    char buffer[128];

    while (true) {
        menu();
        if (!std::fgets(buffer, sizeof(buffer), stdin)) break;
        opcion = std::atoi(buffer);

        if (opcion == 0) {
            break;
        }
        else if (opcion == 1) {
            char id[64];
            printf("ID del sensor de temperatura: ");
            if (!std::fgets(id, sizeof(id), stdin)) continue;
            size_t l = std::strlen(id);
            if (l && (id[l-1] == '\n' || id[l-1] == '\r')) id[l-1] = '\0';
            SensorBase* s = new SensorTemperatura(id);
            gestion.push_back(s);
            printf("Sensor '%s' (Temp) creado e insertado en la lista de gestion.\n", id);
        }
        else if (opcion == 2) {
            char id[64];
            printf("ID del sensor de presion: ");
            if (!std::fgets(id, sizeof(id), stdin)) continue;
            size_t l = std::strlen(id);
            if (l && (id[l-1] == '\n' || id[l-1] == '\r')) id[l-1] = '\0';
            SensorBase* s = new SensorPresion(id);
            gestion.push_back(s);
            printf("Sensor '%s' (Presion) creado e insertado en la lista de gestion.\n", id);
        }
        else if (opcion == 3) {
            char id[64], val[64];
            printf("ID del sensor: ");
            if (!std::fgets(id, sizeof(id), stdin)) continue;
            size_t l = std::strlen(id);
            if (l && (id[l-1] == '\n' || id[l-1] == '\r')) id[l-1] = '\0';

            SensorBase* s = gestion.buscarPorNombre(id);
            if (!s) {
                printf("No existe el sensor '%s'.\n", id);
                continue;
            }

            printf("Valor (float para Temp, int para Presion): ");
            if (!std::fgets(val, sizeof(val), stdin)) continue;
            l = std::strlen(val);
            if (l && (val[l-1] == '\n' || val[l-1] == '\r')) val[l-1] = '\0';

            if (s->registrarDesdeTexto(val)) {
                printf("Lectura registrada en %s.\n", s->getNombre());
            } else {
                printf("Formato de lectura invalido para el tipo de sensor.\n");
            }
        }
        else if (opcion == 4) {
            gestion.procesarTodos();
        }
        else if (opcion == 5) {
            gestion.imprimirResumen();
        }
        else if (opcion == 6) {
            char linea[128];
            printf("Linea (ID,valor): ");
            if (!std::fgets(linea, sizeof(linea), stdin)) continue;
            bool ok = procesarLineaSerial(linea, gestion);
            printf("Inyeccion %s.\n", ok ? "OK" : "fallida");
        }
        else {
            printf("Opcion invalida.\n");
        }
    }

    // Al salir, ~ListaGeneral libera en cascada.
    return 0;
}
