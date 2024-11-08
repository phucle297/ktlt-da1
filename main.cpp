#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define MAX_LINE 1024
#define MAX_FIELDS 20
#define MAX_INTERESTS 5
#define MAX_BASE_HTML_SIZE 4096
#define PATH_TO_CSV "../data.csv"
#define PATH_OUTPUT "../src/"
#define PATH_TO_BASE_HTML "../src/base.html"

typedef struct {
    char id[20];
    char name[100];
    char department[100];
    char email[100];
    char birth[20];
    char image[50];
    char description[200];
    char interests[MAX_INTERESTS][100];
    int interest_count;
} SinhVien;

void parse_csv_line(char *line, SinhVien *student) {
    char *token;
    int field = 0;
    student->interest_count = 0;

    token = strtok(line, ";");
    while (token != NULL && field < MAX_FIELDS) {
        token[strcspn(token, "\r\n")] = 0; // Remove newline characters
        if (token[0] == '"') {
            token++; // Remove leading quote
            token[strlen(token) - 1] = '\0'; // Remove trailing quote
        }

        switch (field) {
            case 0:
                strcpy(student->id, token);
                break;
            case 1:
                strcpy(student->name, token);
                break;
            case 2:
                strcpy(student->department, token);
                break;
            case 3:
                strcpy(student->email, token);
                break;
            case 4:
                strcpy(student->birth, token);
                break;
            case 5:
                strcpy(student->image, token);
                break;
            case 6:
                strcpy(student->description, token);
                break;
            default:
                if (strlen(token) > 0) {
                    strcpy(student->interests[student->interest_count++], token);
                }
                break;

        }
        token = strtok(NULL, ";");
        field++;
    }
}

char *read_base_html(const char *filename) {
    FILE *file = fopen(filename, "rt");
    if (file == NULL) {
        printf("Error opening base HTML file.\n");
        return NULL;
    }

    char *baseHtml = (char *) malloc(MAX_BASE_HTML_SIZE);
    if (baseHtml == NULL) {
        printf("Memory allocation failed.\n");
        fclose(file);
        return NULL;
    }
    size_t len = fread(baseHtml, 1, MAX_BASE_HTML_SIZE - 1, file);
    baseHtml[len] = '\0';

    fclose(file);
    return baseHtml;
}

void replace_placeholder(char **content, const char *placeholder, const char *value) {
    if (!content || !*content || !placeholder || !value) {
        return;
    }

    /* Count occurrences and calculate new size */
    const char *search_pos = *content;
    size_t count = 0;
    size_t placeholder_len = strlen(placeholder);
    size_t value_len = strlen(value);
    size_t content_len = strlen(*content);

    while ((search_pos = strstr(search_pos, placeholder)) != NULL) {
        count++;
        search_pos += placeholder_len;
    }

    if (count == 0) {
        return;  /* No replacements needed */
    }

    /* Calculate new size and allocate memory */
    size_t new_size = content_len + (count * (value_len - placeholder_len)) + 1;
    char *new_content = (char *) malloc(new_size);
    if (!new_content) {
        return;  /* Memory allocation failed */
    }

    /* Perform replacements */
    const char *read_pos = *content;
    char *write_pos = new_content;

    while (1) {
        const char *match_pos = strstr(read_pos, placeholder);
        if (match_pos == NULL) {
            /* Copy remaining content and break */
            strcpy(write_pos, read_pos);
            break;
        }

        /* Copy chunk before placeholder */
        size_t chunk_size = match_pos - read_pos;
        memcpy(write_pos, read_pos, chunk_size);
        write_pos += chunk_size;

        /* Copy replacement value */
        memcpy(write_pos, value, value_len);
        write_pos += value_len;

        /* Move read position */
        read_pos = match_pos + placeholder_len;
    }

    /* Free old content and update pointer */
    free(*content);
    *content = new_content;
}


void generate_interests_html(SinhVien *student, char *interests_html) {
    if (student->interest_count > 0) {
        strcat(interests_html, "<section class=\"interests\">\n");
        strcat(interests_html, "<h2>Sở thích</h2>\n");
        strcat(interests_html, "<ul>\n");
        for (int i = 0; i < student->interest_count; i++) {
            char interest_item[200];
            snprintf(interest_item, sizeof(interest_item), "<li>%s</li>\n", student->interests[i]);
            strcat(interests_html, interest_item);
        }
        strcat(interests_html, "</ul>\n");
        strcat(interests_html, "</section>\n");
    }
}

void generate_html(SinhVien *student, const char *baseHtml, const char *output_path) {
    char *content = strdup(baseHtml);

    replace_placeholder(&content, "{{STUDENT_NAME}}", student->name);
    replace_placeholder(&content, "{{STUDENT_ID}}", student->id);
    replace_placeholder(&content, "{{DEPARTMENT}}", student->department);
    replace_placeholder(&content, "{{EMAIL}}", student->email);
    replace_placeholder(&content, "{{BIRTH_DATE}}", student->birth);
    replace_placeholder(&content, "{{STUDENT_IMAGE}}", student->image);
    replace_placeholder(&content, "{{DESCRIPTION}}", student->description);

    char interests_html[1024] = "";
    generate_interests_html(student, interests_html);
    replace_placeholder(&content, "{{INTERESTS}}", interests_html);

    char filename[256];
    snprintf(filename, sizeof(filename), "%s%s.html", output_path, student->id);

    FILE *file = fopen(filename, "wt");
    if (file == NULL) {
        printf("Error creating output file for student %s.\n", student->id);
        free(content);
        return;
    }

    fprintf(file, "%s", content);
    fclose(file);
    printf("Generated HTML file for student %s: %s\n", student->id, filename);

    free(content);
}

int main() {
    // Create output directory if it doesn't exist
    struct stat st = {0};
    if (stat(PATH_OUTPUT, &st) == -1) {
        mkdir(PATH_OUTPUT);
    }

    char *baseHtml = read_base_html(PATH_TO_BASE_HTML);
    if (baseHtml == NULL) return 1;

    FILE *file = fopen(PATH_TO_CSV, "rt");
    if (file == NULL) {
        printf("Error opening CSV file.\n");
        free(baseHtml);
        return 1;
    }

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), file)) {
        SinhVien student;
        parse_csv_line(line, &student);
        generate_html(&student, baseHtml, PATH_OUTPUT);
    }

    fclose(file);
    free(baseHtml);
    return 0;
}
