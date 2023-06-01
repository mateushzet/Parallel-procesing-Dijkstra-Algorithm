#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <pvm3.h>

// Struktura reprezentująca wierzchołek grafu
typedef struct Vertex {
    int id;
    int distance;
    int visited;
    int previousId;
    struct Vertex* previous;
} Vertex;

int main() {
    int mytid, parenttid;
    int numVertices;
    Vertex* currentVertex;
    int* graph;
    Vertex* vertices;
    int i;

    // Inicjalizacja PVM
    mytid = pvm_mytid();
    parenttid = pvm_parent();

    // Odbieranie danych od mastera
    pvm_recv(parenttid, 1);
    pvm_upkint(&i, 1, 1);
    pvm_upkint(&numVertices, 1, 1);

    currentVertex = (Vertex*)malloc(sizeof(Vertex));
    graph = (int*)malloc(numVertices * numVertices * sizeof(int));
    vertices = (Vertex*)malloc(numVertices * sizeof(Vertex));

    pvm_upkbyte((char*)currentVertex, sizeof(Vertex), 1);
    pvm_upkint(graph, numVertices * numVertices, 1);
    pvm_upkbyte((char*)vertices, numVertices * sizeof(Vertex), 1);

    // Przetwarzanie danych
    int weight = graph[currentVertex->id * numVertices + i];
    if (weight > 0 && !vertices[i].visited && currentVertex->distance != INT_MAX &&
        currentVertex->distance + weight < vertices[i].distance) {
        vertices[i].distance = currentVertex->distance + weight;
        vertices[i].previous = currentVertex;
	vertices[i].previousId = currentVertex->id;
    }

    // Odesłanie danych do mastera
    pvm_initsend(PvmDataDefault);
    pvm_pkint(&i, 1, 1);
    pvm_pkbyte((char*)vertices, numVertices * sizeof(Vertex), 1);
    pvm_send(parenttid, 1);

    // Zwolnienie pamięci
    free(currentVertex);
    free(graph);
    free(vertices);

    // Zakończenie PVM
    pvm_exit();
    return 0;
}