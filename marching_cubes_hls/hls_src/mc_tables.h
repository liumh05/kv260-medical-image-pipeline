#ifndef MC_TABLES_H
#define MC_TABLES_H

// Edge table: 256个配置，每个配置标记哪些边被等值面穿过
extern const int edgeTable[256];

// Triangle table: 256个配置，每个配置最多5个三角形（15个顶点）
extern const int triTable[256][16];

#endif
