#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_JOBS   500
#define MAX_LINE   512
#define DATA_FILE  "../data/jobs.txt"
#define FIELD_SIZE 128

typedef struct {
    int  id;
    char reg_no[FIELD_SIZE];
    char owner_name[FIELD_SIZE];
    char phone[FIELD_SIZE];
    char engine_no[FIELD_SIZE];
    char service_type[FIELD_SIZE];
    char delivery_date[FIELD_SIZE];
    char status[FIELD_SIZE];
    int  priority;
    char extra[FIELD_SIZE];
    char payment_status[FIELD_SIZE]; //
} JobCard;

static void trim_newline(char *s) {
    size_t len = strlen(s);
    while (len > 0 && (s[len-1] == '\n' || s[len-1] == '\r')) {
        s[--len] = '\0';
    }
}

static void safe_copy(char *dest, const char *src, int dest_size) {
    int i = 0;
    while (i < dest_size - 1 && src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

static int parse_line(const char *line, JobCard *j) {
    char buf[MAX_LINE];
    char *fields[11]; //
    int   nf = 0;
    char *p;

    strncpy(buf, line, MAX_LINE - 1);
    buf[MAX_LINE - 1] = '\0';
    trim_newline(buf);

    p = buf;
    fields[nf++] = p;
    while (*p && nf < 11) {
        if (*p == '|') {
            *p = '\0';
            fields[nf++] = p + 1;
        }
        p++;
    }

    if (nf < 10) return 0; 

    j->id = atoi(fields[0]);
    safe_copy(j->reg_no,        fields[1], FIELD_SIZE);
    safe_copy(j->owner_name,    fields[2], FIELD_SIZE);
    safe_copy(j->phone,         fields[3], FIELD_SIZE);
    safe_copy(j->engine_no,     fields[4], FIELD_SIZE);
    safe_copy(j->service_type,  fields[5], FIELD_SIZE);
    safe_copy(j->delivery_date, fields[6], FIELD_SIZE);
    safe_copy(j->status,        fields[7], FIELD_SIZE);
    j->priority = atoi(fields[8]);
    safe_copy(j->extra,         fields[9], FIELD_SIZE);

    if (nf == 11) { //
        safe_copy(j->payment_status, fields[10], FIELD_SIZE);
    } else {
        safe_copy(j->payment_status, "unpaid", FIELD_SIZE);
    }
    return 1;
}

int fh_read_all(JobCard *jobs, int max) {
    FILE *fp = fopen(DATA_FILE, "r");
    int count = 0;
    char line[MAX_LINE];
    if (!fp) return 0;
    while (count < max && fgets(line, sizeof(line), fp)) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
        if (parse_line(line, &jobs[count])) count++;
    }
    fclose(fp);
    return count;
}

int fh_write_all(JobCard *jobs, int count) {
    FILE *fp = fopen(DATA_FILE, "w");
    int i; // Declared outside for loop
    if (!fp) return 0;
    fprintf(fp, "# Two-Wheeler Service Jobs\n");
    fprintf(fp, "# ID|RegNo|OwnerName|Phone|EngineNo|ServiceType|DeliveryDate|Status|Priority|Extra|PaymentStatus\n"); //
    for (i = 0; i < count; i++) {
        fprintf(fp, "%d|%s|%s|%s|%s|%s|%s|%s|%d|%s|%s\n",
            jobs[i].id, jobs[i].reg_no, jobs[i].owner_name,
            jobs[i].phone, jobs[i].engine_no, jobs[i].service_type,
            jobs[i].delivery_date, jobs[i].status, jobs[i].priority,
            jobs[i].extra, jobs[i].payment_status); //
    }
    fclose(fp);
    return 1;
}

int calc_priority(const char *delivery_date) {
    struct tm due = {0};
    time_t now, due_t;
    if (sscanf(delivery_date, "%d-%d-%d", &due.tm_year, &due.tm_mon, &due.tm_mday) != 3) return 0;
    due.tm_year -= 1900; due.tm_mon -= 1; due.tm_isdst = -1;
    now = time(NULL);
    due_t = mktime(&due);
    return (int)(difftime(due_t, now) / 86400.0);
}

int main(int argc, char *argv[]) {
    if (argc >= 2 && strcmp(argv[1], "daemon") == 0) {
        JobCard mem_jobs[MAX_JOBS];
        int mem_count = fh_read_all(mem_jobs, MAX_JOBS);
        char line[MAX_LINE];
        int i; // Declared for loops
        
        while (fgets(line, sizeof(line), stdin)) {
            trim_newline(line);
            if (strcmp(line, "EXIT") == 0) {
                fh_write_all(mem_jobs, mem_count);
                break;
            } 
            /* ADD COMMAND */
            else if (strncmp(line, "ADD|", 4) == 0) {
                char *p = line + 4;
                char *fields[10];
                int nf = 0, max_id = 0;
                fields[nf++] = p;
                while (*p && nf < 10) {
                    if (*p == '|') { *p = '\0'; fields[nf++] = p + 1; }
                    p++;
                }
                if (nf >= 6) { 
                    JobCard j; memset(&j, 0, sizeof(j));
                    for (i = 0; i < mem_count; i++) if (mem_jobs[i].id > max_id) max_id = mem_jobs[i].id;
                    j.id = max_id + 1;
                    safe_copy(j.reg_no, fields[0], FIELD_SIZE);
                    safe_copy(j.owner_name, fields[1], FIELD_SIZE);
                    safe_copy(j.phone, fields[2], FIELD_SIZE);
                    safe_copy(j.engine_no, fields[3], FIELD_SIZE);
                    safe_copy(j.service_type, fields[4], FIELD_SIZE);
                    safe_copy(j.delivery_date, fields[5], FIELD_SIZE);
                    safe_copy(j.status, "pending", FIELD_SIZE);
                    safe_copy(j.extra, (nf >= 7 && *fields[6]) ? fields[6] : "—", FIELD_SIZE);
                    safe_copy(j.payment_status, "unpaid", FIELD_SIZE); //
                    j.priority = calc_priority(j.delivery_date);
                    if (mem_count < MAX_JOBS) { 
                        mem_jobs[mem_count++] = j; 
                        fh_write_all(mem_jobs, mem_count);
                        printf("Added job %d successfully.\n", j.id); 
                    } else printf("Error: limit reached.\n");
                }
                fflush(stdout);
            } 
            /* UPDATE COMMAND */
            else if (strncmp(line, "UPDATE|", 7) == 0) {
                char *p = line + 7; char *fields[3]; int nf = 0, id, found = 0; 
                fields[nf++] = p;
                while (*p && nf < 3) { if (*p == '|') { *p = '\0'; fields[nf++] = p + 1; } p++; }
                if (nf >= 2) {
                    id = atoi(fields[0]);
                    for (i = 0; i < mem_count; i++) {
                        if (mem_jobs[i].id == id) {
                            if (fields[1] && *fields[1]) safe_copy(mem_jobs[i].status, fields[1], FIELD_SIZE);
                            if (nf >= 3 && fields[2] && *fields[2]) safe_copy(mem_jobs[i].extra, fields[2], FIELD_SIZE);
                            mem_jobs[i].priority = calc_priority(mem_jobs[i].delivery_date);
                            found = 1; break;
                        }
                    }
                    if (found) {
                        fh_write_all(mem_jobs, mem_count);
                        printf("Updated job %d successfully.\n", id);
                    } else printf("Error: id not found.\n");
                }
                fflush(stdout);
            }
            /* UPDATE_PAYMENT COMMAND */
            else if (strncmp(line, "UPDATE_PAYMENT|", 15) == 0) { //
                char *p = line + 15;
                char *fields[2]; int nf = 0, id, found = 0; 
                fields[nf++] = p;
                while (*p && nf < 2) { if (*p == '|') { *p = '\0'; fields[nf++] = p + 1; } p++; }
                if (nf >= 2) {
                    id = atoi(fields[0]);
                    for (i = 0; i < mem_count; i++) {
                        if (mem_jobs[i].id == id) {
                            safe_copy(mem_jobs[i].payment_status, fields[1], FIELD_SIZE); //
                            found = 1; break;
                        }
                    }
                    if (found) {
                        fh_write_all(mem_jobs, mem_count);
                        printf("successfully updated payment\n");
                    } else printf("Error: id not found\n");
                }
                fflush(stdout);
            } 
            /* GET_ALL COMMAND */
            else if (strcmp(line, "GET_ALL") == 0) {
                for (i = 0; i < mem_count; i++) {
                    printf("%d|%s|%s|%s|%s|%s|%s|%s|%d|%s|%s\n", 
                        mem_jobs[i].id, mem_jobs[i].reg_no, mem_jobs[i].owner_name, 
                        mem_jobs[i].phone, mem_jobs[i].engine_no, mem_jobs[i].service_type, 
                        mem_jobs[i].delivery_date, mem_jobs[i].status, mem_jobs[i].priority, 
                        mem_jobs[i].extra, mem_jobs[i].payment_status); //
                }
                printf("END_GET_ALL\n"); fflush(stdout);
            }
        }
        return 0;
    }
    return 0;
}
