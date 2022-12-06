#include "simplify.h"
#define TOLIT(x) ((x) > 0 ? (x) : ((-x) + vars))
#define NEG(x) ((x) > vars ? ((x) - vars) : ((x) + vars))

simplify::simplify():
  vars                  (0),
  clauses               (0),
  maxlen                (0)
{}


inline int pnsign(int x) {
    return (x > 0 ? 1 : -1);
}
inline int sign(int x) {
    return x % 2 == 1 ? -1 : 1;
}
inline int tolit(int x) {
    if (x > 0) return x * 2;
    else return (-x) * 2 + 1;
}
inline int toidx(int x) {
    return (x & 1 ? -(x >> 1) : (x >> 1));
}
inline int reverse(int x) {
    if (x % 2 == 1) return x - 1;
    else return x + 1;
}
inline ll simplify::mapv(int a, int b) {
    return 1ll * a * nlit + (ll)b;
}
int simplify::find(int x) {
    if (f[x] == x) return x;
    int fa = f[x];
    f[x] = find(fa);
    val[x] = val[fa] * val[x];
    return f[x];
}

void simplify::simplify_init() {
    f = new int[vars + 10];
    val = new int[vars + 10];
    color = new int[vars + 10];
    varval = new int[vars + 10];
    q = new int[vars + 10];
    clean = new int[vars + 10];
    seen = new int[(vars << 1) + 10];
    clause_delete.growTo(clauses+1, 0);
    nxtc.growTo(clauses+1, 0);
    occurp = new svec<int>[vars + 1];
    occurn = new svec<int>[vars + 1]; 
    for (int i = 1; i <= clauses; i++) {
        int l = clause[i].size();
        maxlen = max(maxlen, l);
    }
    resseen = new int[(vars << 1) + 10];
    a = new int[maxlen + 1];

    mapval = new int[vars + 10];
    mapto = new int[vars + 10];
    for (int i = 1; i <= vars; i++) mapto[i] = i, mapval[i] = 0;
}

void simplify::release() {
    delete []f;
    delete []val;
    delete []color;
    delete []varval;
    delete []q;
    delete []clean;
    delete []seen;
    clause_delete.clear(true);
    nxtc.clear(true);
    delete []resseen;   
    delete []a;
    delete []mapfrom; 
    for (int i = 0; i <= vars; i++)
        occurp[i].clear(true), occurn[i].clear(true);
    delete []occurp;
    delete []occurn;
    C.clear();
}


bool simplify::res_is_empty(int x) {
    int op = occurp[x].size(), on = occurn[x].size();
    for (int i = 0; i < op; i++) {
        int o1 = occurp[x][i], l1 = clause[o1].size();
        if (clause_delete[o1]) continue;
        for (int j = 0; j < l1; j++)
            if (abs(clause[o1][j]) != x) resseen[abs(clause[o1][j])] = pnsign(clause[o1][j]);
        for (int j = 0; j < on; j++) {
            int o2 = occurn[x][j], l2 = clause[o2].size(), flag = 0;
            if (clause_delete[o2]) continue;
            for (int k = 0; k < l2; k++)
                if (abs(clause[o2][k]) != x && resseen[abs(clause[o2][k])] == -pnsign(clause[o2][k])) {
                    flag = 1; break;
                }
            if (!flag) {
                for (int j = 0; j < l1; j++)
                    resseen[abs(clause[o1][j])] = 0;
                return false;
            }
        }
        for (int j = 0; j < l1; j++)
            resseen[abs(clause[o1][j])] = 0;
    }
    return true; 
}

bool simplify::simplify_resolution() {
    
    for (int i = 1; i <= vars; i++) {
        occurn[i].clear();
        occurp[i].clear();
        resseen[i] = resseen[i + vars] = clean[i] = seen[i] = 0;
    }
    for (int i = 1; i <= clauses; i++) {
        int l = clause[i].size();
        clause_delete[i] = 0;
        for (int j = 0; j < l; j++)
            if (clause[i][j] > 0) occurp[abs(clause[i][j])].push(i);
            else occurn[abs(clause[i][j])].push(i);
    }
    for (int i = 1; i <= vars; i++)
        if (occurn[i].size() == 0 && occurp[i].size() == 0) clean[i] = 1;  

    int l = 1, r = 0;         
    for (int i = 1; i <= vars; i++) {
        int op = occurp[i].size(), on = occurn[i].size();
        if (op * on > op + on || clean[i]) continue;
        if (res_is_empty(i)) {
            q[++r] = i, clean[i] = 1;
        }
    }

    int now_turn = 0, seen_flag = 0;
    svec<int> vars;
    while (l <= r) {
        ++now_turn;
        for (int j = l; j <= r; j++) {
            int i = q[j];
            int op = occurp[i].size(), on = occurn[i].size();
            for (int j = 0; j < op; j++) clause_delete[occurp[i][j]] = 1;
            for (int j = 0; j < on; j++) clause_delete[occurn[i][j]] = 1;
        }
        int ll = l; l = r + 1;
        
        vars.clear();
        ++seen_flag;
        for (int u = ll; u <= r; u++) {
            int i = q[u];
            int op = occurp[i].size(), on = occurn[i].size();
            for (int j = 0; j < op; j++) {
                int o = occurp[i][j], l = clause[o].size();
                for (int k = 0; k < l; k++) {
                    int v = abs(clause[o][k]);
                    if (!clean[v] && seen[v] != seen_flag)
                        vars.push(v), seen[v] = seen_flag;
                }
            }
            for (int j = 0; j < on; j++) {
                int o = occurn[i][j], l = clause[o].size();
                for (int k = 0; k < l; k++) {
                    int v = abs(clause[o][k]);
                    if (!clean[v] && seen[v] != seen_flag)
                        vars.push(v), seen[v] = seen_flag;
                }
            }
        }
        for (int u = 0; u < vars.size(); u++) {
            int i = vars[u];
            int op = 0, on = 0;
            for (int j = 0; j < occurp[i].size(); j++) op += 1 - clause_delete[occurp[i][j]];
            for (int j = 0; j < occurn[i].size(); j++) on += 1 - clause_delete[occurn[i][j]];
            if (op * on > op + on) continue;
            if (res_is_empty(i)) {
                q[++r] = i, clean[i] = 1;
            }
        }
    }
    vars.clear(true);
    if (!r) return true;
    res_clauses = 0;
    res_clause.push();
    for (int i = 1; i <= clauses; i++) {
        if (!clause_delete[i]) continue;
        ++res_clauses;
        res_clause.push();
        int l = clause[i].size();
        for (int j = 0; j < l; j++) {
            res_clause[res_clauses].push(pnsign(clause[i][j]) * mapfrom[abs(clause[i][j])]);
        }
    }
    resolutions = r;
    resolution.push();
    for (int i = 1; i <= r; i++) {
        int v = mapfrom[q[i]];
        resolution.push(v);
        mapto[v] = 0, mapval[v] = -10;
    }
    update_var_clause_label();
    for (int i = 1; i <= orivars; i++) {
        if (mapto[i]) {
            mapto[i] = color[mapto[i]];
        }
    }
    return true;
}

void simplify::update_var_clause_label() {
    int remain_var = 0;
    for (int i = 1; i <= vars; i++) color[i] = 0;
    for (int i = 1; i <= clauses; i++) {
        if (clause_delete[i]) continue;
        int l = clause[i].size();
        for (int j = 0; j < l; j++) {
            if (color[abs(clause[i][j])] == 0) color[abs(clause[i][j])] = ++remain_var;       
        }
    }

    int id = 0;
    for (int i = 1; i <= clauses; i++) {
        if (clause_delete[i]) {clause[i].setsize(0); continue;}
        ++id;
        int l = clause[i].size();
        if (i == id) {
            for (int j = 0; j < l; j++)
                clause[id][j] = color[abs(clause[i][j])] * pnsign(clause[i][j]);    
            continue;
        }
        clause[id].setsize(0);
        for (int j = 0; j < l; j++)
            clause[id].push(color[abs(clause[i][j])] * pnsign(clause[i][j]));
    }
    // printf("c After simplify: vars: %d -> %d , clauses: %d -> %d ,\n", vars, remain_var, clauses, id);
    for (int i = id + 1; i <= clauses; i++) 
        clause[i].clear(true);
    for (int i = remain_var + 1; i <= vars; i++)
        occurp[i].clear(true), occurn[i].clear(true);
    clause.setsize(id + 1);
    vars = remain_var, clauses = id;
}

bool simplify::simplify_binary() {
    for (int i = 1; i <= clauses; i++) {
        int l = clause[i].size();
        for (int j = 0; j < l; j++)
            clause[i][j] = tolit(clause[i][j]);
    }
    nlit = 2 * vars + 2;
    int simplify = 1, turn = 0;
    for (int i = 1; i <= vars; i++) f[i] = i, val[i] = 1, varval[i] = color[i] = 0;
    for (int i = 1; i <= clauses; i++) clause_delete[i] = 0;

    int len = 0;
    for (int i = 1; i <= clauses; i++) {
        if (clause[i].size() != 2) continue;
        nxtc[++len] = i;
        ll id1 = mapv(clause[i][0], clause[i][1]),
           id2 = mapv(clause[i][1], clause[i][0]);
        C.insert(id1, i);
        C.insert(id2, i);
    }
    
    while (simplify) {
        simplify = 0;
        ++turn;        
        for (int k = 1; k <= len; k++) {
            int i = nxtc[k];
            if (clause[i].size() != 2 || clause_delete[i]) continue;
            ll id1 = mapv(reverse(clause[i][0]), reverse(clause[i][1])),
               id2 = mapv(clause[i][0], reverse(clause[i][1])),
               id3 = mapv(reverse(clause[i][0]), clause[i][1]);
            int r = C.get(id1, 0);
            if (r) {
                simplify = 1;
                clause_delete[r] = clause_delete[i] = 1;
                int fa = find(clause[i][0] >> 1), fb = find(clause[i][1] >> 1);
                int sig = sign(clause[i][0]) * sign(clause[i][1]) * (-1);
                //sig == 1 : a = b   -1 : a = -b
                if (fa != fb) {
                    if (fa < fb) {
                        f[fa] = fb;
                        val[fa] = sig / (val[clause[i][0] >> 1] * val[clause[i][1] >> 1]);
                        if (varval[fa])
                            varval[fb] = val[fa] * varval[fa];
                    }
                    else if (fa > fb) {
                        f[fb] = fa;
                        val[fb] = sig / (val[clause[i][0] >> 1] * val[clause[i][1] >> 1]);
                        if (varval[fb])
                            varval[fa] = val[fb] * varval[fb];
                    }
                }
                else {
                    if (sig != val[clause[i][0] >> 1] * val[clause[i][1] >> 1])
                        return false;
                }
            }
            int d1 = C.get(id2, 0);
            if (d1) {
                int v = clause[i][0] >> 1;
                if (varval[v] && varval[v] != sign(clause[i][0])) {
                    return false;
                }
                clause_delete[d1] = clause_delete[i] = 1;
                simplify = 1;
                varval[v] = sign(clause[i][0]);
            }
            int d2 = C.get(id3, 0);
            if (d2) {
                int v = clause[i][1] >> 1;
                if (varval[v] && varval[v] != sign(clause[i][1])) {
                    return false;
                }
                clause_delete[d2] = clause_delete[i] = 1;
                simplify = 1;
                varval[v] = sign(clause[i][1]); 
            }
        }
        
        for (int i = 1; i <= vars; i++) {
            int x = find(i);
            if (varval[i] && x != i) {
                if (varval[x]) {
                    if (varval[x] != varval[i] * val[i])
                        return false;
                }
                else varval[x] = varval[i] * val[i];
            }
        }
        for (int i = 1; i <= vars; i++) 
            if (varval[f[i]]) {
                if (varval[i]) {
                    if (varval[f[i]] != varval[i] * val[i])
                        return false;
                }
                else varval[i] = varval[f[i]] * val[i];
            }

        len = 0;

        for (int i = 1; i <= clauses; i++) {
            if (clause_delete[i]) continue;
            int l = clause[i].size(), oril = l;
            for (int j = 0; j < l; j++) {
                int fa = f[clause[i][j] >> 1];
                a[j] = tolit(sign(clause[i][j]) * val[clause[i][j] >> 1] * fa);
            }
            int t = 0;
            for (int j = 0; j < l; j++) {
                int x = varval[a[j] >> 1];
                if (x) {
                    int k = x * sign(a[j]);
                    if (k == 1) {
                        if (!clause_delete[i]) simplify = 1;
                        clause_delete[i] = 1, a[t++] = a[j];
                    }
                }
                else a[t++] = a[j];
            }
            if (t == 0) return false;
            if (t != l) simplify = 1, l = t;
            t = 0;
            for (int j = 0; j < l; j++) {
                if (resseen[a[j]] == i) continue;
                resseen[a[j]] = i, a[t++] = a[j];
            }
            if (t != l) simplify = 1, l = t;
            for (int j = 0; j < l; j++)
                if (resseen[reverse(a[j])] == i && !clause_delete[i])
                    clause_delete[i] = 1, simplify = 1;
            
            for (int j = 0; j < l; j++) resseen[a[j]] = 0;
                
            if (l == 1) {
                if (sign(a[0]) * varval[a[0] >> 1] == -1) return false;
                varval[a[0] >> 1] = sign(a[0]);
                simplify = 1;
            }
            if (!clause_delete[i] && l == 2 && oril != 2) {
                nxtc[++len] = i;
                ll id1 = mapv(a[0], a[1]),
                   id2 = mapv(a[1], a[0]);
                C.insert(id1, i);
                C.insert(id2, i);
            }
            else if (!clause_delete[i] && l == 2 &&  oril == 2) {
                if (a[0] == clause[i][0] && a[1] == clause[i][1]) ;
                else if (a[1] == clause[i][0] && a[0] == clause[i][1]) ;
                else {
                    nxtc[++len] = i;
                    ll id1 = mapv(a[0], a[1]),
                       id2 = mapv(a[1], a[0]);
                    C.insert(id1, i);
                    C.insert(id2, i);
                }
            }
            clause[i].clear();
            for (int j = 0; j < l; j++)
                clause[i].push(a[j]);
        }

        for (int i = 1; i <= vars; i++) {
            int x = find(i);
            if (varval[i] && x != i) {
                if (varval[x]) {
                    if (varval[x] != varval[i] * val[i])
                        return false;
                }
                else varval[x] = varval[i] * val[i];
            }
        }
        for (int i = 1; i <= vars; i++) 
            if (varval[f[i]]) {
                if (varval[i]) {
                    if (varval[f[i]] != varval[i] * val[i])
                        return false;
                }
                else varval[i] = varval[f[i]] * val[i];
            }
    }

    for (int i = 1; i <= clauses; i++) {
        if (clause_delete[i]) continue;
        int l = clause[i].size();
        for (int j = 0; j < l; j++) {
            clause[i][j] = sign(clause[i][j]) * (clause[i][j] >> 1);
        }
    }
    update_var_clause_label();
    for (int i = 1; i <= orivars; i++) {
        if (mapval[i]) {
            continue;
        }
        int v = mapto[i], fa = find(v);
        if (varval[v] || varval[fa]) {
            mapval[i] = varval[v];
            mapto[i] = 0;
        }
        else if (color[fa]) mapto[i] = color[fa] * val[v];
        else mapval[i] = val[v], mapto[i] = 0;
    }
    return true;
}

bool simplify::simplify_easy_clause() {
    for (int i = 1; i <= vars; i++) {
        varval[i] = 0;
        occurp[i].clear();
        occurn[i].clear();
        resseen[i] = resseen[i + vars] = 0;
    }
    for (int i = 1; i <= clauses; i++) clause_delete[i] = 0;
    int head = 1, tail = 0;
    for (int i = 1; i <= clauses; i++) {
        int l = clause[i].size(), t = 0;
        for (int j = 0; j < l; j++) {
            int lit = TOLIT(clause[i][j]);
            if (resseen[lit] == i) continue;
            if (resseen[NEG(lit)] == i) {clause_delete[i] = 1; break;}
            clause[i][t++] = clause[i][j];
            resseen[lit] = i;
        }
        if (clause_delete[i]) continue;
        clause[i].setsize(t);
        for (int j = 0; j < t; j++) 
            if (clause[i][j] > 0) occurp[clause[i][j]].push(i);
            else occurn[-clause[i][j]].push(i);
        if (t == 0) return false;
        if (t == 1) {
            int lit = clause[i][0];
            clause_delete[i] = 1;
            if (varval[abs(lit)]) {
                if (varval[abs(lit)] == pnsign(lit)) continue;
                else return false;
            }
            varval[abs(lit)] = pnsign(lit); 
            q[++tail] = abs(lit); 
        }
    }
    for (int i = 1; i <= vars + vars; i++) resseen[i] = 0;
    while (head <= tail) {
        int x = q[head++];
        if (varval[x] == 1) {
            for (int i = 0; i < occurp[x].size(); i++)
                clause_delete[occurp[x][i]] = 1;
            for (int i = 0; i < occurn[x].size(); i++) {
                int o = occurn[x][i], t = 0;
                if (clause_delete[o]) continue;
                for (int j = 0; j < clause[o].size(); j++) {
                    if (varval[abs(clause[o][j])] == pnsign(clause[o][j])) {
                        clause_delete[o] = 1; break;
                    }
                    if (varval[abs(clause[o][j])] == -pnsign(clause[o][j])) continue;
                    clause[o][t++] = clause[o][j];
                }
                if (clause_delete[o]) continue;
                clause[o].setsize(t);
                if (t == 0) return false;
                if (t == 1) {
                    int lit = clause[o][0];
                    clause_delete[o] = 1;
                    if (varval[abs(lit)]) {
                        if (varval[abs(lit)] == pnsign(lit)) continue;
                        else return false;
                    }
                    varval[abs(lit)] = pnsign(lit); 
                    q[++tail] = abs(lit); 
                }
            }
        }
        else {
            for (int i = 0; i < occurn[x].size(); i++)
                clause_delete[occurn[x][i]] = 1;
            for (int i = 0; i < occurp[x].size(); i++) {
                int o = occurp[x][i], t = 0;
                if (clause_delete[o]) continue;
                for (int j = 0; j < clause[o].size(); j++) {
                    if (varval[abs(clause[o][j])] == pnsign(clause[o][j])) {
                        clause_delete[o] = 1; break;
                    }
                    if (varval[abs(clause[o][j])] == -pnsign(clause[o][j])) continue;
                    clause[o][t++] = clause[o][j];
                }
                if (clause_delete[o]) continue;
                clause[o].setsize(t);
                if (t == 0) return false;
                if (t == 1) {
                    int lit = clause[o][0];
                    clause_delete[o] = 1;
                    if (varval[abs(lit)]) {
                        if (varval[abs(lit)] == pnsign(lit)) continue;
                        else return false;
                    }
                    varval[abs(lit)] = pnsign(lit); 
                    q[++tail] = abs(lit); 
                }
            }
        }
    }
    update_var_clause_label();

    for (int i = 1; i <= tail; i++) {
        int v = q[i];
        mapval[v] = varval[v];
    }
    mapfrom = new int[vars + 1];
    for (int i = 1; i <= vars; i++) mapfrom[i] = 0;
    for (int i = 1; i <= orivars; i++) {
        if (color[i])
            mapto[i] = color[i], mapfrom[color[i]] = i;
        else if (!mapval[i]) // not in unit queue, then it is no use var
            mapto[i] = 0, mapval[i] = 1;
        else
            mapto[i] = 0;
    }
    return true;
}

void simplify::print_complete_model() {
    int r = 0;
    for (int i = 1; i <= orivars; i++) 
        if (!mapto[i]) {
            if (!mapval[i]);
            else if (abs(mapval[i]) != 1) mapval[i] = 0, ++r;
        }
    if (r) { 
        occurp = new svec<int>[orivars + 1];
        occurn = new svec<int>[orivars + 1];   
        for (int i = 1; i <= orivars; i++) {
            occurp[i].clear(), occurn[i].clear();
        }
        svec<int> clause_state;
        clause_state.growTo(res_clauses + 1, 0);
        for (int i = 1; i <= res_clauses; i++) {
            int satisify = 0;
            for (int j = 0; j < res_clause[i].size(); j++) {
                int v = res_clause[i][j];
                if (v > 0) occurp[v].push(i);
                else occurn[-v].push(i);
                if (pnsign(v) * mapval[abs(v)] == 1) satisify = 1;
                if (!mapval[abs(v)]) ++clause_state[i];
            }
            if (satisify) clause_state[i] = -1;
        }
        for (int ii = resolutions; ii >= 1; ii--) {
            int v = resolution[ii];
            //attempt 1
            int assign = 1;
            for (int i = 0; i < occurn[v].size(); i++) {
                int o = occurn[v][i];
                if (clause_state[o] != -1 && clause_state[o] <= 1) {assign = 0; break;}
            }
            if (assign == 1) {
                mapval[v] = 1;
                for (int i = 0; i < occurn[v].size(); i++) {
                    int o = occurn[v][i];
                    if (clause_state[o] != -1) clause_state[o]--;
                } 
                for (int i = 0; i < occurp[v].size(); i++) 
                    clause_state[occurp[v][i]] = -1;
                continue;
            }
            //attempt -1
            assign = -1;
            for (int i = 0; i < occurp[v].size(); i++) {
                int o = occurp[v][i];
                if (clause_state[o] != -1 && clause_state[o] <= 1) {assign = 0; break;}
            }
            if (assign == -1) {
                mapval[v] = -1;
                for (int i = 0; i < occurp[v].size(); i++) {
                    int o = occurp[v][i];
                    if (clause_state[o] != -1) clause_state[o]--;
                } 
                for (int i = 0; i < occurn[v].size(); i++) 
                    clause_state[occurn[v][i]] = -1;
                continue;
            }
        }
        clause_state.clear(true);
        for (int i = 1; i <= orivars; i++) {
            occurp[i].clear(true), occurn[i].clear(true);
        }
        delete []occurp;
        delete []occurn;
        res_clause.clear(true);
        resolution.clear(true);
    }  
}