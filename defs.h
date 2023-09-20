#define INTERACTIVE 0
#define BATCH 1
#define PARALLEL_DELIMITER '&'
#define REDIRECTION_DELIMITER '>'
#define PATH '/bin'
#define CMD_NODE_STRUCT(T) \
struct Node_##T {         \
    T* data;               \
    struct Node_##T* next; \
};
#define CMD_TOKENS_STRUCT(T) \
struct Tokens_##T {             \
  T* cmd;                       \
  struct Node_##T* token;       \
  int len;                      \
};
#define EXIT "exit"