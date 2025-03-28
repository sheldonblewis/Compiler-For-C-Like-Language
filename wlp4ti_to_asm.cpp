#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <map>

using namespace std;

struct ASTNode {
    string label;
    vector<ASTNode*> kids;
    string type;

    ASTNode(const string& lbl) : label(lbl) {}
    ~ASTNode() { for (ASTNode* k : kids) delete k; }

    ASTNode* find(string val, int idx = 1) {
        int cnt = 0;
        for (ASTNode* k : kids) if (k->label == val && ++cnt == idx) return k;
        return nullptr;
    }
};

map<string, pair<string, int>> symbols;
int condCount = 0;

bool isNum(const string& s) {
    for (char c : s) if (!isdigit(c)) return false;
    return true;
}

bool isTerminal(const string& s) {
    if (isNum(s) || s == "NULL" || s == "BOF" || s == "EOF" || s == ".EMPTY" || s == "GETCHAR" || !iswalnum(s[0])) return true;
    for (char c : s) if (!islower(c)) return false;
    return false;
}

ASTNode* findUnexpanded(ASTNode* root, const string& label) {
    if (root->label == label && root->kids.empty()) return root;
    if (isTerminal(root->label)) return nullptr;
    for (ASTNode* k : root->kids) {
        ASTNode* r = findUnexpanded(k, label);
        if (r) return r;
    }
    return nullptr;
}

ASTNode* buildAST() {
    string line;
    vector<pair<string, vector<string>>> rules;
    while (getline(cin, line) && !line.empty()) {
        istringstream ss(line);
        string parent, token;
        ss >> parent;
        vector<string> children;
        while (ss >> token) children.push_back(token);
        rules.emplace_back(parent, children);
    }

    ASTNode* root = new ASTNode(rules[0].first);
    for (auto& [p, cs] : rules) {
        ASTNode* tgt = findUnexpanded(root, p);
        if (!tgt) continue;
        bool typeFlag = false;
        ASTNode* node;
        for (string& c : cs) {
            if (c == ":") typeFlag = true;
            else if (typeFlag) { node->type = c; tgt->type = c; typeFlag = false; }
            else { node = new ASTNode(c); tgt->kids.push_back(node); }
        }
    }
    return root;
}

void collect(ASTNode* n, const string& label, vector<ASTNode*>& out) {
    if (!n) return;
    if (n->label == label) out.push_back(n);
    for (ASTNode* c : n->kids) collect(c, label, out);
}

void pushReg(int r) {
    cout << "sw $" << r << ", -4($30)\nsub $30, $30, $4\n";
}

void popReg(int r) {
    cout << "add $30, $30, $4\nlw $" << r << ", -4($30)\n";
}

void callFunc(const string& name) {
    pushReg(31);
    cout << "lis $3\n.word " << name << "\njalr $3\n";
    popReg(31);
}

void generate(ASTNode* n) {
    if (!n) return;
    string val = n->label;

    if (val == "ID") {
        int off = symbols[n->kids.front()->label].second;
        cout << "lw $3, " << off << "($29)\n";
    } else if (val == "NUM") {
        cout << "lis $3\n.word " << n->kids.front()->label << "\n";
    } else if (val == "dcls") {
        if (n->kids.size() > 1) {
            generate(n->find("dcls"));
            cout << "lis $3\n.word " << n->find("NUM")->kids.front()->label << "\nsw $3, -4($30)\nsub $30, $30, $4\n";
        }
    } else if (val == "lvalue") {
        if (n->find("ID")) {
            string id = n->find("ID")->kids.front()->label;
            int off = symbols[id].second;
            cout << "sw $3, " << off << "($29)\n";
        } else generate(n->find("lvalue"));
    } else if (val == "expr") {
        if (n->kids.size() == 1) {
            generate(n->find("term"));
        } else {
            // expr → expr PLUS/MINUS term
            generate(n->kids[0]);       // left
            pushReg(3);
            generate(n->kids[2]);       // right
            popReg(5);
            string op = n->kids[1]->label;
            if (op == "PLUS")      cout << "add $3, $5, $3\n";
            else if (op == "MINUS")cout << "sub $3, $5, $3\n";
        }
    } else if (val == "term") {
        if (n->kids.size() == 1) {
            generate(n->find("factor"));
        } else {
            // term → term STAR/SLASH/PCT factor
            generate(n->kids[0]);       // left
            pushReg(3);
            generate(n->kids[2]);       // right
            popReg(5);
            string op = n->kids[1]->label;
            if (op == "STAR")      cout << "mult $5, $3\nmflo $3\n";
            else if (op == "SLASH")cout << "div $5, $3\nmflo $3\n";
            else                   cout << "div $5, $3\nmfhi $3\n";
        }
    } else if (val == "test") {
        string op = n->kids[1]->label;
        generate(n->kids[0]); pushReg(3); generate(n->kids[2]); popReg(5);
        int id = ++condCount;
        if (op == "EQ" || op == "NE") {
            cout << (op == "EQ" ? "beq" : "bne") << " $3, $5, cond" << id << "\nadd $3, $0, $0\nbeq $0, $0, end" << id << "\ncond" << id << ":\nlis $3\n.word 1\nend" << id << ":\n";
        } else if (op == "LT") cout << "slt $3, $5, $3\n";
        else if (op == "GT") cout << "slt $3, $3, $5\n";
        else {
            cout << (op == "LE" ? "slt $3, $3, $5" : "slt $3, $5, $3") << "\nlis $5\n.word 1\nsub $3, $5, $3\n";
        }
    } else if (val == "factor") {
        if (n->find("GETCHAR")) {
            cout << "lis $5\n.word 0xffff0004\nlw $3, 0($5)\n";
        } else if (n->find("ID")) {
            generate(n->find("ID"));
        } else if (n->find("NUM")) {
            generate(n->find("NUM"));
        } else {
            generate(n->find("expr"));  // handle parenthesized expr
        }
    } else if (val == "statements") {
        if (n->kids.size() > 1) { generate(n->find("statements")); generate(n->find("statement")); }
    } else if (val == "statement") {
        if (n->find("PRINTLN")) { generate(n->find("expr")); pushReg(1); cout << "add $1, $3, $0\n"; callFunc("print"); popReg(1); }
        else if (n->find("PUTCHAR")) { generate(n->find("expr")); cout << "lis $5\n.word 0xffff000c\nsw $3, 0($5)\n"; }
        else if (n->kids[1]->label == "BECOMES") { generate(n->find("expr")); generate(n->find("lvalue")); }
        else if (n->kids[0]->label == "IF") {
            int id = ++condCount;
            generate(n->find("test"));
            cout << "beq $0, $3, else" << id << "\n";
            generate(n->find("statements", 1));
            cout << "beq $0, $0, endif" << id << "\nelse" << id << ":\n";
            generate(n->find("statements", 2));
            cout << "endif" << id << ":\n";
        } else if (n->kids[0]->label == "WHILE") {
            int id = ++condCount;
            cout << "while" << id << ":\n";
            generate(n->find("test"));
            cout << "lis $5\n.word 1\nbne $5, $3, endw" << id << "\n";
            generate(n->find("statements"));
            cout << "beq $0, $0, while" << id << "\nendw" << id << ":\n";
        }
    }
}

int main() {
    ASTNode* tree = buildAST();
    ASTNode* mainFn = tree->find("procedures")->find("main");
    vector<ASTNode*> decls;
    collect(tree, "dcl", decls);
    // Assign offsets: params first (positive), locals next (negative)
    int paramCount = 2; // wain always has two parameters

    int paramOffset = 0;
    for (int i = 0; i < paramCount; ++i) {
        ASTNode* d = decls[i];
        string name = d->find("ID")->kids.front()->label;
        symbols[name] = { d->find("ID")->kids.front()->type, paramOffset };
        paramOffset += 4;
    }

    int numLocals = decls.size() - paramCount;
    for (int i = 0; i < numLocals; ++i) {
        ASTNode* d = decls[paramCount + i];
        string name = d->find("ID")->kids.front()->label;
        // Topmost local (last pushed) gets offset 0($29), others are above
        int offset = 4 * (numLocals - i - 1);
        symbols[name] = { d->find("ID")->kids.front()->type, offset };
    }

    cout << ".import print\n";
    cout << "lis $4\n.word 4\n";
    cout << "sw $2, -4($30)\nsub $30, $30, $4\n";  // push b
    cout << "sw $1, -4($30)\nsub $30, $30, $4\n";  // push a

    generate(mainFn->find("dcls"));               // push locals

    // Set $29 = $30 + 8 using two adds
    cout << "add $29, $30, $0\n";
    
    generate(mainFn->find("statements"));
    generate(mainFn->find("expr"));
    numLocals = decls.size() - 2;
    for (int i = 0; i < numLocals; ++i) {
        cout << "add $30, $30, $4\n";
    }
    cout << "jr $31\n";
    delete tree;
    return 0;
}
