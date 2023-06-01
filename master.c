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

void printVertices(const Vertex* vertices, int numVertices) {
    for (int i = 0; i < numVertices; i++) {
        printf("Vertex ID: %d\n", vertices[i].id);
        printf("Distance: %d\n", vertices[i].distance);
        printf("Visited: %d\n", vertices[i].visited);
	printf("previous: %d\n", vertices[i].previousId);

        printf("--------------------\n");
   	
	 }
}

void printOne(const Vertex* vertex) {
    printf("Vertex ID: %d\n", vertex->id);
    printf("Distance: %d\n", vertex->distance);
    printf("Visited: %d\n", vertex->visited);
    printf("Previous Vertex: %d\n", vertex->previous->id);
}


// Funkcja do inicjalizacji wierzchołków grafu
void initializeVertices(Vertex** vertices, int numVertices, int source) {
    for (int i = 0; i < numVertices; i++) {
        (*vertices)[i].id = i;
        (*vertices)[i].distance = INT_MAX;
        (*vertices)[i].visited = 0;
        (*vertices)[i].previous = NULL;
	(*vertices)[i].previousId = -1;    
}
    (*vertices)[source].distance = 0;
}



// Funkcja do znalezienia wierzchołka o najmniejszej odległości
int findMinDistanceVertex(Vertex* vertices, int numVertices) {
    int minDistance = INT_MAX;
    int minIndex = -1;

    for (int i = 0; i < numVertices; i++) {
        if (!vertices[i].visited && vertices[i].distance < minDistance) {
            minDistance = vertices[i].distance;
            minIndex = i;
        }
    }

    return minIndex;
}



// Funkcja do relaksacji krawędzi wychodzących z wierzchołka
void relaxEdges(Vertex* currentVertex, int* graph, Vertex* vertices, int numVertices) {

    // Inicjalizacja PVM
    int tids[numVertices];
    int mytid = pvm_mytid();
    pvm_spawn("slave", NULL, PvmTaskDefault, "", numVertices, tids);

    for (int i = 0; i < numVertices; i++) {

        // Wysłanie danych do slave'a
        pvm_initsend(PvmDataDefault);
        pvm_pkint(&i, 1, 1);
        pvm_pkint(&numVertices, 1, 1);
        pvm_pkbyte((char*)currentVertex, sizeof(Vertex), 1);
        pvm_pkint(graph, numVertices * numVertices, 1);
        pvm_pkbyte((char*)vertices, numVertices * sizeof(Vertex), 1);
        pvm_send(tids[i], 1);
    }

    for (int i = 0; i < numVertices; i++) {
        // Odbieranie danych od slave'a
        pvm_recv(tids[i], 1);
        int x;
        pvm_upkint(&x, 1, 1);
        Vertex* v = (Vertex*)malloc(numVertices * sizeof(Vertex));
	pvm_upkbyte((char*)v, numVertices *  sizeof(Vertex), 1);
	vertices[x] = v[x];
	//printOne(&v[x]);
}

//printf("odbeiram");


}



// Funkcja do wypisania najkrótszej ścieżki
void printShortestPath(const Vertex* vertices, int id) {
    if (id == -1 ) return;

    printShortestPath(vertices, vertices[id].previousId);
    printf("%d ", vertices[id].id);
}

int* getShortestPath(const Vertex* vertices, int numVertices, int source, int destination, int *pathLength) {
    int *path = malloc(numVertices * sizeof(int));
    *pathLength = 0;
    int currentId = destination;

    while (currentId != source) {
        path[(*pathLength)++] = currentId;
        currentId = vertices[currentId].previousId;
    }

    path[(*pathLength)++] = source;

    // Odwrócenie kolejności wierzchołków na ścieżce
    int *reversedPath = malloc((*pathLength) * sizeof(int));
    for (int i = 0; i < (*pathLength); i++) {
        reversedPath[i] = path[(*pathLength) - 1 - i];
    }

    free(path);

    return reversedPath;
}


int isPathEdge(int vertex1, int vertex2, int *path, int pathLength) {
    for (int i = 0; i < pathLength - 1; i++) {
        if ((path[i] == vertex1 && path[i + 1] == vertex2) || (path[i] == vertex2 && path[i + 1] == vertex1)) {
            return 1;
        }
    }
    return 0;
}


void visualizeGraph(int *graph, int numVertices, int *path, int pathLength) {
    FILE *fp;
    fp = fopen("graph.dot", "w");

    if (fp == NULL) {
        printf("Błąd podczas tworzenia pliku graph.dot.\n");
        return;
    }

    fprintf(fp, "graph G {\n");

    for (int i = 0; i < numVertices; i++) {
        for (int j = i + 1; j < numVertices; j++) {  // Iteracja tylko po poniżej/diagonalnej głównej części macierzy
            if (graph[i * numVertices + j] != 0) {
                if (isPathEdge(i, j, path, pathLength)) { // Sprawdzenie, czy krawędź należy do ścieżki
                    fprintf(fp, "  %d -- %d [color=\"red\", label=\"%d\"];\n", i, j, graph[i * numVertices + j]);
                } else {
                    fprintf(fp, "  %d -- %d [label=\"%d\"];\n", i, j, graph[i * numVertices + j]);
                }
            }
        }
    }

    fprintf(fp, "}\n");
    fclose(fp);

    system("dot -Tpng graph.dot -o graph.png");
}


// Funkcja algorytmu Dijkstry
void DijkstraAlgorithm(int* graph, int numVertices, int source, int destination) {
    Vertex* vertices = (Vertex*)malloc(numVertices * sizeof(Vertex));
    initializeVertices(&vertices, numVertices, source);

    for (int count = 0; count < numVertices - 1; count++) {
        int currentVertexIndex = findMinDistanceVertex(vertices, numVertices);
        Vertex* currentVertex = &vertices[currentVertexIndex];
        currentVertex->visited = 1;
        relaxEdges(currentVertex, graph, vertices, numVertices);
	
    }
    printf("Najkrótsza ścieżka z wierzchołka %d do wierzchołka %d: ", source, destination);
 printShortestPath(vertices, destination);

printf("\n");

int pathLength;
int* path = getShortestPath(vertices, numVertices, source, destination, &pathLength);

visualizeGraph(graph, numVertices, path, pathLength);

free(path);



    free(vertices);
}


int main() {
    FILE* file = fopen("/home/ubuntu/pvm3/src/dane6.txt", "r");
    if (file == NULL) {
        printf("Nie można otworzyć pliku.\n");
        return 1;
    }



//    int* graph;
    int numVertices, source, destination;


    // Odczyt zmiennych numVertices, source i destination
    fscanf(file, "liczba Wierzcholkow = %d;\n", &numVertices);
    fscanf(file, "start = %d;\n", &source);
    fscanf(file, "koniec = %d;\n", &destination);
 
int graphSize = numVertices*numVertices;
int graph[graphSize];

    // Odczyt tablicy graph
    fscanf(file, "graf:\n");
    for (int i = 0; i < graphSize; i++) {
        fscanf(file, "%d, ", &graph[i]);
    }
    fscanf(file, "\n};\n");

 

 

    fclose(file);

 

    DijkstraAlgorithm(graph, numVertices, source, destination);

 

    return 0;
}