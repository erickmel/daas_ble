#ifdef NODEA
#define localDIN 120
#define remoteDIN 130
#define local_uri "abcd1234"
#define remote_uri "dcba4321"
#elifdef NODEB
#define localDIN 130
#define remoteDIN 120
#define local_uri "dcba4321"
#define remote_uri "abcd1234"
#endif