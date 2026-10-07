/*
 * Transaction State Manager
 * -------------------------
 * A command-line program that stores transactions and manages their state.
 *
 * State flow:  CREATED -> PROCESSING -> SUCCESS or FAILED
 * SUCCESS and FAILED are final states (no further change allowed).
 *
 * Commands: CREATE <id> <amount> | START <id> | SUCCESS <id> | FAIL <id>
 *           STATUS <id> | LIST | EXIT
 */

#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TRANSACTIONS 1000   /* requirement: at least 100 */
#define MAX_ID_LEN       32     /* includes the '\0' terminator */
#define LINE_SIZE        256

/* ---------- Data types ---------- */

typedef enum {
    STATE_CREATED,
    STATE_PROCESSING,
    STATE_SUCCESS,
    STATE_FAILED
} TxnState;

typedef struct {
    char     id[MAX_ID_LEN];
    double   amount;
    TxnState state;
} Transaction;

/* All transactions live in this fixed-size array. */
static Transaction transactions[MAX_TRANSACTIONS];
static int         txn_count = 0;

/* ---------- Helpers ---------- */

static const char *state_to_string(TxnState s)
{
    switch (s) {
        case STATE_CREATED:    return "CREATED";
        case STATE_PROCESSING: return "PROCESSING";
        case STATE_SUCCESS:    return "SUCCESS";
        case STATE_FAILED:     return "FAILED";
    }
    return "UNKNOWN";
}

/* Print 500 as "500" and 500.5 as "500.50". */
static void print_amount(double amount)
{
    if (amount == floor(amount) && amount < 1e15)
        printf("%.0f", amount);
    else
        printf("%.2f", amount);
}

/* Returns the index of the transaction with this id, or -1 if not found. */
static int find_transaction(const char *id)
{
    for (int i = 0; i < txn_count; i++) {
        if (strcmp(transactions[i].id, id) == 0)
            return i;
    }
    return -1;
}

/* Parse a positive, finite number. Returns false if text is not valid. */
static bool parse_amount(const char *text, double *out)
{
    char *end = NULL;
    double value = strtod(text, &end);

    if (end == text || *end != '\0') return false;  /* not a full number */
    if (!isfinite(value))            return false;  /* rejects nan / inf */
    if (value <= 0)                  return false;  /* must be > 0       */

    *out = value;
    return true;
}

/* ---------- Main transaction operations ---------- */

static void create_transaction(const char *id, const char *amount_text)
{
    double amount;

    if (strlen(id) >= MAX_ID_LEN) {
        printf("Error: transaction id too long (max %d characters)\n", MAX_ID_LEN - 1);
        return;
    }
    if (!parse_amount(amount_text, &amount)) {
        printf("Error: amount must be a number greater than 0\n");
        return;
    }
    if (find_transaction(id) != -1) {
        printf("Error: transaction %s already exists\n", id);
        return;
    }
    if (txn_count >= MAX_TRANSACTIONS) {
        printf("Error: storage full (max %d transactions)\n", MAX_TRANSACTIONS);
        return;
    }

    Transaction *t = &transactions[txn_count++];
    strcpy(t->id, id);
    t->amount = amount;
    t->state  = STATE_CREATED;
    printf("Transaction %s created\n", id);
}

static void start_transaction(const char *id)
{
    int idx = find_transaction(id);
    if (idx == -1) {
        printf("Error: transaction %s not found\n", id);
        return;
    }
    if (transactions[idx].state != STATE_CREATED) {
        printf("Error: invalid state change\n");
        return;
    }
    transactions[idx].state = STATE_PROCESSING;
    printf("Transaction %s is PROCESSING\n", id);
}

/* Shared by SUCCESS and FAIL: only PROCESSING can move to a final state. */
static void finish_transaction(const char *id, TxnState final_state)
{
    int idx = find_transaction(id);
    if (idx == -1) {
        printf("Error: transaction %s not found\n", id);
        return;
    }
    if (transactions[idx].state != STATE_PROCESSING) {
        printf("Error: invalid state change\n");
        return;
    }
    transactions[idx].state = final_state;
    printf("Transaction %s is %s\n", id, state_to_string(final_state));
}

static void print_status(const char *id)
{
    int idx = find_transaction(id);
    if (idx == -1) {
        printf("Error: transaction %s not found\n", id);
        return;
    }
    printf("%s | ", transactions[idx].id);
    print_amount(transactions[idx].amount);
    printf(" | %s\n", state_to_string(transactions[idx].state));
}

static void list_transactions(void)
{
    if (txn_count == 0) {
        printf("No transactions\n");
        return;
    }
    for (int i = 0; i < txn_count; i++)
        print_status(transactions[i].id);
}

/* ---------- Command handling ---------- */

/* Returns false when the program should stop (EXIT). */
static bool handle_command(char *line)
{
    char *tokens[4];
    int   n = 0;

    /* Split the line into words (max 4 kept, the rest only counted). */
    for (char *tok = strtok(line, " \t\r\n"); tok != NULL; tok = strtok(NULL, " \t\r\n")) {
        if (n < 4) tokens[n] = tok;
        n++;
    }
    if (n == 0) return true;                       /* blank line: ignore */

    /* Commands are case-insensitive; ids are case-sensitive. */
    for (char *p = tokens[0]; *p; p++)
        *p = (char)toupper((unsigned char)*p);
    const char *cmd = tokens[0];

    if (strcmp(cmd, "CREATE") == 0) {
        if (n != 3) printf("Error: usage: CREATE <id> <amount>\n");
        else        create_transaction(tokens[1], tokens[2]);
    } else if (strcmp(cmd, "START") == 0) {
        if (n != 2) printf("Error: usage: START <id>\n");
        else        start_transaction(tokens[1]);
    } else if (strcmp(cmd, "SUCCESS") == 0) {
        if (n != 2) printf("Error: usage: SUCCESS <id>\n");
        else        finish_transaction(tokens[1], STATE_SUCCESS);
    } else if (strcmp(cmd, "FAIL") == 0) {
        if (n != 2) printf("Error: usage: FAIL <id>\n");
        else        finish_transaction(tokens[1], STATE_FAILED);
    } else if (strcmp(cmd, "STATUS") == 0) {
        if (n != 2) printf("Error: usage: STATUS <id>\n");
        else        print_status(tokens[1]);
    } else if (strcmp(cmd, "LIST") == 0) {
        if (n != 1) printf("Error: usage: LIST\n");
        else        list_transactions();
    } else if (strcmp(cmd, "EXIT") == 0) {
        return false;
    } else {
        printf("Error: invalid command\n");
    }
    return true;
}

int main(void)
{
    char line[LINE_SIZE];

    while (fgets(line, sizeof line, stdin) != NULL) {
        /* If the line was longer than the buffer, throw away the rest of it. */
        if (strchr(line, '\n') == NULL && !feof(stdin)) {
            int c;
            while ((c = getchar()) != '\n' && c != EOF) { }
            printf("Error: line too long\n");
            continue;
        }
        if (!handle_command(line))
            break;
    }
    return 0;
}