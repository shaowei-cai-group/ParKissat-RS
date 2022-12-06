#include "simplify.h"
#include <cstring>
#include <fstream>
using namespace std;

char *read_whitespace(char *p) {
    while ((*p >= 9 && *p <= 13) || *p == 32)
        ++p;
    return p;
}
char *read_until_new_line(char *p)
{
    while (*p != '\n')
    {
        if (*p == '\0')
        {
            // printf("c parse error: unexpected EOF");
            exit(0);
        }
        ++p;
    }
    return ++p;
}

char *read_int(char *p, int *i)
{
    *i = 0;
    bool sym = true;
    p = read_whitespace(p);
    if (*p == '-')
        sym = false, ++p;
    while (*p >= '0' && *p <= '9')
    {
        if (*p == '\0')
            return p;
        *i = *i * 10 + *p - '0';
        ++p;
    }
    if (!sym)
        *i = -(*i);
    return p;
}

void simplify::readfile(const char *file) {
    std::string infile(file);
	std::ifstream fin(infile);
    fin.seekg(0, fin.end);
	size_t file_len = fin.tellg();
	fin.seekg(0, fin.beg);
	char *data = new char[file_len + 1];
	fin.read(data, file_len);
	fin.close();
	data[file_len] = '\0';
    char *p = data;
    clause.push();
    clause.push();
    int num_clauses = 1;
    while (*p != '\0')
    {   
        p = read_whitespace(p);
        if (*p == '\0')
            break;
        if (*p == 'c')
            p = read_until_new_line(p);
        else if (*p == 'p')
        {
            p += 5;
            p = read_int(p, &vars);
            p = read_int(p, &clauses);
            orivars = vars;
            oriclauses = clauses; 
        }
        else
        {
            int32_t dimacs_lit;
            p = read_int(p, &dimacs_lit);
            if (*p == '\0' && dimacs_lit != 0)
            {
                // printf("c PARSE ERROR! Unexpected EOF\n");
                exit(0);
            }
            if (dimacs_lit == 0)
                num_clauses += 1, clause.push();
            else
                clause[num_clauses].push(dimacs_lit);
        }
    }
    if (num_clauses != clauses + 1) {
        // printf("c parse warning: clauses: %d, real clauses: %d\n", clauses, num_clauses - 1);
        clauses = num_clauses - 1;
    }
    delete []data;
}
