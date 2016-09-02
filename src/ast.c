/*
The MIT License (MIT)

Copyright (c) 2015 - Latino

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
#include <math.h>
#include <stdbool.h>

#include "latino.h"
#include "ast.h"
#include "vm.h"
#include "object.h"
#include "libmem.h"
#include "libstring.h"
#include "liblist.h"
#include "libio.h"

#define dbc(I, A, B, M) bcode[i++] = lat_bc(I, A, B, M)
#define pn(vm, N) i = nodo_analizar(vm, N, bcode, i)
#define fdbc(I, A, B, M) funcion_bcode[fi++] = lat_bc(I, A, B, M)
#define fpn(vm, N) fi = nodo_analizar(vm, N, funcion_bcode, fi)

ast *nodo_nuevo(nodo_tipo nt, ast *l, ast *r)
{
    ast *a = (ast*)__memoria_asignar(sizeof(ast));
    a->tipo = nt;
    a->izq = l;
    a->der = r;
    a->valor = NULL;
    return a;
}

ast *nodo_nuevo_decimal(double d, int num_linea, int num_columna)
{
    ast *a = (ast*)__memoria_asignar(sizeof(ast));
    a->tipo = NODO_VALOR;
    nodo_valor *val = (nodo_valor*)__memoria_asignar(sizeof(nodo_valor));
    val->tipo = VALOR_NUMERICO;
    val->val.numerico = d;
    a->valor = val;
    a->num_linea = num_linea;
    a->num_columna = num_columna;
    return a;
}

ast *nodo_nuevo_logico(int b, int num_linea, int num_columna)
{
    ast *a = (ast*)__memoria_asignar(sizeof(ast));
    a->tipo = NODO_VALOR;
    nodo_valor *val = (nodo_valor*)__memoria_asignar(sizeof(nodo_valor));
    val->tipo = VALOR_LOGICO;
    val->val.logico = b;
    a->valor = val;
    a->num_linea = num_linea;
    a->num_columna = num_columna;
    return a;
}

ast *nodo_nuevo_cadena(const char *s, int num_linea, int num_columna)
{
    ast *a = (ast*)__memoria_asignar(sizeof(ast));
    a->tipo = NODO_VALOR;
    nodo_valor *val = (nodo_valor*)__memoria_asignar(sizeof(nodo_valor));
    val->tipo = VALOR_CADENA;
    val->val.cadena = __str_analizar(s, strlen(s));
    a->valor = val;
    a->num_linea = num_linea;
    a->num_columna = num_columna;
    return a;
}

ast *nodo_nuevo_identificador(const char *s, int num_linea, int num_columna)
{
    ast *a = (ast*)__memoria_asignar(sizeof(ast));
    a->tipo = NODO_IDENTIFICADOR;
    nodo_valor *val = (nodo_valor*)__memoria_asignar(sizeof(nodo_valor));
    val->tipo = VALOR_CADENA;
    val->val.cadena = __str_duplicar(s);
    a->valor = val;
    a->num_linea = num_linea;
    a->num_columna = num_columna;
    return a;
}

ast *nodo_nuevo_asignacion(ast *v, ast *s)
{
    ast *a = (ast*)__memoria_asignar(sizeof(ast));
    a->tipo = NODO_ASIGNACION;
    a->izq = v;
    a->der = s;
    a->valor = NULL;
    return a;
}

ast *nodo_nuevo_asignacion_lista_elem(ast *exp, ast *id, ast *pos)
{
    nodo_lista_elem *a = (nodo_lista_elem*)__memoria_asignar(sizeof(nodo_lista_elem));
    a->tipo = NODO_LISTA_ASIGNAR_ELEMENTO;
    a->expresion = exp;
    a->identificador = id;
    a->posicion = pos;
    return (ast *)a;
}

ast *nodo_nuevo_asignacion_dicc_elem(ast *exp, ast *id, ast *llave)
{
    nodo_lista_elem *a = (nodo_lista_elem*)__memoria_asignar(sizeof(nodo_lista_elem));
    a->tipo = NODO_DICC_ASIGNAR_ELEMENTO;
    a->expresion = exp;
    a->identificador = id;
    a->posicion = llave;
    return (ast *)a;
}

ast *nodo_nuevo_si(ast *cond, ast *th, ast *el)
{
    nodo_si *a = (nodo_si*)__memoria_asignar(sizeof(nodo_si));
    a->tipo = NODO_SI;
    a->condicion = cond;
    a->entonces = th;
    a->_sino = el;
    return (ast *)a;
}

ast *nodo_nuevo_mientras(ast *cond, ast *stmts)
{
    ast *a = (ast*)__memoria_asignar(sizeof(ast));
    a->tipo = NODO_MIENTRAS;
    a->izq = cond;
    a->der = stmts;
    a->valor = NULL;
    return a;
}

ast *nodo_nuevo_hacer(ast *cond, ast *stmts)
{
    ast *a = (ast*)__memoria_asignar(sizeof(ast));
    a->tipo = NODO_HACER;
    a->izq = cond;
    a->der = stmts;
    a->valor = NULL;
    return a;
}

ast *nodo_nuevo_desde(ast *dec, ast *cond, ast *inc, ast *stmts)
{
    ast *a = (ast*)__memoria_asignar(sizeof(ast));
    a->tipo = NODO_BLOQUE;
    a->izq = dec;
    a->der = nodo_nuevo_mientras(cond, nodo_nuevo(NODO_BLOQUE, stmts, inc));
    a->valor = NULL;
    return a;
}

ast *nodo_nuevo_funcion(ast *s, ast *syms, ast *func)
{
    ast *a = (ast*)__memoria_asignar(sizeof(ast));
    a->tipo = NODO_ASIGNACION;
    a->izq = nodo_nuevo(NODO_FUNCION_USUARIO, syms, func);
    a->der = s;
    a->valor = NULL;
    return a;
}

ast* nodo_nuevo_incluir(ast* ruta)
{
    ast *a = (ast*)__memoria_asignar(sizeof(ast));
    a->tipo = NODO_INCLUIR;
    a->izq = ruta;
    a->der = NULL;
    a->valor = NULL;
    return a;
}

ast* nodo_reducir_constantes(nodo_tipo nt, ast* lhs, ast* rhs){
    double tmp = 0;
    //operadores unarios
    if(lhs->tipo == NODO_VALOR  && lhs->valor->tipo != VALOR_NUMERICO){
        lat_error("Linea %d, %d: %s", lhs->num_linea, lhs->num_columna, "Valor numerico requerido");
        return NULL;
    }
    if(rhs == NULL){
        if(nt == NODO_MAS_UNARIO){
            return nodo_nuevo(NODO_MAS_UNARIO, lhs, NULL);
        }
        if(nt == NODO_MENOS_UNARIO){
            return nodo_nuevo(NODO_MENOS_UNARIO, lhs, NULL);
        }
    }
    /*operadores binarios*/
    if(lhs->tipo == NODO_VALOR  && rhs->valor->tipo != VALOR_NUMERICO){
        lat_error("Linea %d, %d: %s", rhs->num_linea, rhs->num_columna, "Valor numerico requerido");
        return NULL;
    }
    if(lhs->tipo == NODO_VALOR && rhs->tipo == NODO_VALOR){
        switch(nt){
            case NODO_SUMA:
                tmp = lhs->valor->val.numerico + rhs->valor->val.numerico;
                break;
            case NODO_RESTA:
                tmp = lhs->valor->val.numerico - rhs->valor->val.numerico;
                break;
            case NODO_MULTIPLICACION:
                tmp = lhs->valor->val.numerico * rhs->valor->val.numerico;
                break;
            case NODO_DIVISION:{
                if(rhs->valor->val.numerico == 0){
                    lat_error("Linea %d, %d: %s", rhs->num_linea, rhs->num_columna, "Division entre cero");
                    return NULL;
                }
                tmp = lhs->valor->val.numerico / rhs->valor->val.numerico;
            }
                break;
            case NODO_MODULO:{
                if(rhs->valor->val.numerico == 0){
                    lat_error("Linea %d, %d: %s", rhs->num_linea, rhs->num_columna, "Modulo entre cero");
                    return NULL;
                }
                tmp = fmod(lhs->valor->val.numerico, rhs->valor->val.numerico);
            }
                break;
            default:
                if(!parse_silent){
                    lat_error("Linea %d, %d: %s", lhs->num_linea, lhs->num_columna, "Operador inválido");
                    return NULL;
                }
        }
        return nodo_nuevo_decimal(tmp, lhs->num_linea, lhs->num_columna);
    }
    return nodo_nuevo(nt, lhs, rhs);
}

void nodo_liberar(ast *a)
{
    if (a)
    {
        switch (a->tipo)
        {
        case NODO_SI:
        {
            nodo_si* nsi = (nodo_si*) a;
            nodo_liberar(nsi->condicion);
            nodo_liberar(nsi->entonces);
            if (nsi->_sino)
                nodo_liberar(nsi->_sino);
            break;
        }
        case NODO_LISTA_ASIGNAR_ELEMENTO:
        {
            nodo_lista_elem* nelem = (nodo_lista_elem*)a;
            nodo_liberar(nelem->expresion);
            nodo_liberar(nelem->identificador);
            nodo_liberar(nelem->posicion);
            break;
        }
        case NODO_DICC_ASIGNAR_ELEMENTO:
        {
            nodo_dicc_elem* nelem = (nodo_dicc_elem*)a;
            nodo_liberar(nelem->expresion);
            nodo_liberar(nelem->identificador);
            nodo_liberar(nelem->llave);
            break;
        }
        case NODO_IDENTIFICADOR:
        case NODO_VALOR:
            if(a->valor->tipo == VALOR_CADENA){
            __memoria_liberar(a->valor->val.cadena);
            }
            __memoria_liberar(a->valor);
        break;
        default:
            nodo_liberar(a->izq);
            nodo_liberar(a->der);
        }
        __memoria_liberar(a);
    }
}

ast* transformar_casos(ast* casos, ast* cond_izq)
{
    if(casos == NULL)
    {
        return NULL;
    }
    ast* caso = casos->izq;
    ast* cond = NULL;
    if(caso->tipo == NODO_CASO)
    {
        cond = nodo_nuevo(NODO_IGUALDAD, cond_izq, caso->izq);
    }
    if(caso->tipo == NODO_DEFECTO)
    {
        cond = nodo_nuevo(NODO_IGUALDAD, cond_izq, cond_izq);
    }
    ast* nSi = nodo_nuevo_si(cond, caso->der, ((ast*)transformar_casos(casos->der, cond_izq)));
    return nSi;
}

static ast* __transformar_elegir(ast* nodo_elegir)
{
    ast* cond_izq = nodo_elegir->izq;
    ast* casos = nodo_elegir->der;
    ast* nSi = NULL;
    nSi = transformar_casos(casos, cond_izq);
    return nSi;
}

int nested = -1;
static int nodo_analizar(lat_vm *vm, ast *node, lat_bytecode *bcode, int i)
{
    int temp[8] = {0};
    lat_bytecode *funcion_bcode = NULL;
    int fi = 0;
    switch (node->tipo)
    {
    case NODO_INCLUIR:
    {
        /*
        //TODO: Incluir rutas con punto ej. incluir "lib.modulos.myModulo"
        char* archivo = node->izq->valor->val.cadena;
        lat_objeto* mod = lat_cadena_nueva(vm, archivo);
        if (!__lista_contiene_valor(vm->modulos, (void*)mod))
        {
            //encontrar el modulo en la ruta actual
            char dir_actual[MAX_PATH_LENGTH];
            getcwd(dir_actual, sizeof(dir_actual));
            strcat(dir_actual, PATH_SEP);
            strcat(dir_actual, archivo);
            if (!__str_termina_con(dir_actual, ".lat"))
            {
                strcat(dir_actual, ".lat");
            }
            if (__io_es_legible(dir_actual))
            {
                __lista_apilar(vm->modulos, mod);
                pn(vm, lat_analizar_archivo(dir_actual));
            }
            else
            {
                //sino existe buscar en el path_home de latino/lib
                char* latino_lib = getenv("LATINO_LIB");
                if (latino_lib != NULL)
                {
                    strcat(latino_lib, PATH_SEP);
                    strcat(latino_lib, archivo);
                    if (!__str_termina_con(latino_lib, ".lat"))
                    {
                        strcat(latino_lib, ".lat");
                    }
                    if (__io_es_legible(latino_lib))
                    {
                        __lista_apilar(vm->modulos, mod);
                        pn(vm, lat_analizar_archivo(latino_lib));
                    }
                }
            }
        }*/
    }
    break;
    case NODO_BLOQUE:
    {
        if (node->izq)
        {
            pn(vm, node->izq);
        }
        if (node->der)
        {
            pn(vm, node->der);
        }
    }
    break;
    case NODO_IDENTIFICADOR: /*GET: Obtiene el valor de la variable en la tabla de simbolos*/
    {
        lat_objeto *o = lat_cadena_nueva(vm, node->valor->val.cadena);
        o->num_linea = node->num_linea;
        o->num_columna = node->num_columna;
        dbc(LOAD_NAME, 0, 0, o);
    }
    break;
    case NODO_ASIGNACION: /*SET: Asigna el valor de la variable en la tabla de simbolos*/
    {
        pn(vm, node->izq);
        lat_objeto *o = lat_cadena_nueva(vm, node->der->valor->val.cadena);
        o->num_linea = node->der->num_linea;
        o->num_columna = node->der->num_columna;
        dbc(STORE_NAME, 0, 0, o);
    }
    break;
    case NODO_VALOR:
    {
        lat_objeto *o = NULL;
        if(node->valor->tipo == VALOR_LOGICO)
            o = (node->valor->val.logico == true) ? vm->objeto_verdadero : vm->objeto_falso;
        if(node->valor->tipo == VALOR_NUMERICO)
            o = lat_decimal_nuevo(vm, node->valor->val.numerico);
        if(node->valor->tipo == VALOR_CADENA)
            o = lat_cadena_nueva(vm, node->valor->val.cadena);
        o->num_linea = node->num_linea;
        o->num_columna = node->num_columna;
        dbc(LOAD_CONST, 0, 0, o);
    }
    break;
    case NODO_MAS_UNARIO:
    {
        pn(vm, node->izq);
    }
    break;
    case NODO_MENOS_UNARIO:
    {
        pn(vm, node->izq);
        dbc(UNARY_MINUS, 0, 0, NULL);
    }
    break;
    case NODO_SUMA:
    {
        pn(vm, node->izq);
        pn(vm, node->der);
        dbc(BINARY_ADD, 0, 0, NULL);
    }
    break;
    case NODO_RESTA:
    {
        pn(vm, node->izq);
        pn(vm, node->der);
        dbc(BINARY_SUB, 0, 0, NULL);
    }
    break;
    case NODO_MULTIPLICACION:
    {
        pn(vm, node->izq);
        pn(vm, node->der);
        dbc(BINARY_MUL, 0, 0, NULL);
    }
    break;
    case NODO_DIVISION:
    {
        pn(vm, node->izq);
        pn(vm, node->der);
        dbc(BINARY_DIV, 0, 0, NULL);
    }
    break;
    case NODO_MODULO:
    {
        pn(vm, node->izq);
        pn(vm, node->der);
        dbc(BINARY_MOD, 0, 0, NULL);
    }
    break;
    case NODO_MAYOR_QUE:
    {
        pn(vm, node->izq);
        pn(vm, node->der);
        dbc(OP_GT, 0, 0, NULL);
    }
    break;
    case NODO_MAYOR_IGUAL:
    {
        pn(vm, node->izq);
        pn(vm, node->der);
        dbc(OP_GE, 0, 0, NULL);
    }
    break;
    case NODO_MENOR_QUE:
    {
        pn(vm, node->izq);
        pn(vm, node->der);
        dbc(OP_LT, 0, 0, NULL);
    }
    break;
    case NODO_MENOR_IGUAL:
    {
        pn(vm, node->izq);
        pn(vm, node->der);
        dbc(OP_LE, 0, 0, NULL);
    }
    break;
    case NODO_IGUALDAD:
    {
        pn(vm, node->izq);
        pn(vm, node->der);
        dbc(OP_EQ, 0, 0, NULL);
    }
    break;
    case NODO_DESIGUALDAD:
    {
        pn(vm, node->izq);
        pn(vm, node->der);
        dbc(OP_NEQ, 0, 0, NULL);
    }
    break;
    case NODO_Y:
    {
        pn(vm, node->izq);
        pn(vm, node->der);
        dbc(OP_AND, 0, 0, NULL);
    }
    break;
    case NODO_O:
    {
        pn(vm, node->izq);
        pn(vm, node->der);
        dbc(OP_OR, 0, 0, NULL);
    }
    break;
    case NODO_NO:
    {
        pn(vm, node->izq);
        dbc(OP_NOT, 0, 0, NULL);
    }
    break;
    case NODO_CONCATENAR:
    {
        pn(vm, node->izq);
        pn(vm, node->der);
        dbc(CONCAT, 0, 0, NULL);
    }
    break;
    case NODO_SI:
    {
        nodo_si *nIf = ((nodo_si *)node);
        pn(vm, nIf->condicion);
        temp[0] = i;
        dbc(NOP, 0, 0, NULL);
        pn(vm, nIf->entonces);
        temp[1] = i;
        dbc(NOP, 0, 0, NULL);
        temp[2] = i;
        if(nIf->_sino){
            pn(vm, nIf->_sino);
        }
        temp[3] = i;
        bcode[temp[0]] = lat_bc(POP_JUMP_IF_FALSE, (temp[2]-1), 0, NULL);
        bcode[temp[1]] = lat_bc(JUMP_ABSOLUTE, (temp[3]-1), 0, NULL);
    }
    break;
    case NODO_ELEGIR:
    {
        //transformar nodo elegir en nodos si
        ast* nSi = __transformar_elegir(node);
        pn(vm, nSi);
    }
    break;
    case NODO_MIENTRAS:
    {
        dbc(SETUP_LOOP, 0, 0, NULL);
        temp[0] = i;
        pn(vm, node->izq);
        temp[1] = i;
        dbc(NOP, 0, 0, NULL);
        pn(vm, node->der);
        dbc(JUMP_ABSOLUTE, (temp[0] -1), 0, NULL);
        dbc(POP_BLOCK, 0, 0, NULL);
        bcode[temp[1]] = lat_bc(POP_JUMP_IF_FALSE, (i-1), 0, NULL);
    }
    break;
    case NODO_HACER:
    {
        dbc(SETUP_LOOP, 0, 0, NULL);
        temp[0] = i;
        pn(vm, node->der);
        pn(vm, node->izq);
        temp[1] = i;
        dbc(NOP, 0, 0, NULL);
        dbc(POP_BLOCK, 0, 0, NULL);
        bcode[temp[1]] = lat_bc(POP_JUMP_IF_TRUE, (temp[0]-1), 0, NULL);
    }
    break;
    case NODO_FUNCION_LLAMADA:
    {
        //TODO: primero procesar nombre de funcion
        if (node->der)
        {
            pn(vm, node->der);
        }
        pn(vm, node->izq);
        dbc(CALL_FUNCTION, 0, 0, NULL);
    }
    break;
    case NODO_RETORNO:
    {
        pn(vm, node->izq);
        dbc(RETURN_VALUE, 0, 0, NULL);
    }
    break;
    case NODO_FUNCION_ARGUMENTOS:
    {
        if (node->izq)
        {
            pn(vm, node->izq);
        }
        if (node->der)
        {
            pn(vm, node->der);
            if (node->der->valor || node->der->tipo == NODO_FUNCION_LLAMADA)
            {
                dbc(CALL_FUNCTION, 0, 0, NULL);
            }
        }        
    }
    break;
    case NODO_LISTA_PARAMETROS:
    {
        if (node->izq)
        {            
            lat_objeto *o = lat_cadena_nueva(vm, node->izq->valor->val.cadena);
            dbc(STORE_NAME, 0, 0, o);
        }
        if (node->der)
        {
            pn(vm, node->der);
        }
        /*if (node->izq)
        {
            dbc(OP_LOCALNS, 1, 0, NULL);
            dbc(OP_POP, 2, 0, NULL);
            //lat_objeto *ret = lat_clonar_objeto(vm, lat_cadena_nueva(vm, node->l->valor->v.s));
            lat_objeto *ret = lat_cadena_nueva(vm, node->izq->valor->val.cadena);
            dbc(OP_SET, 1, 2, ret);
#if DEPURAR_AST
            printf("LOCALNS R1\n");
            printf("POP R2\n");
            printf("SET R1 R2 %s\n", ret->data.str);
#endif
        }
        if (node->der)
        {
            pn(vm, node->der);
        }*/
    }
    break;
    case NODO_FUNCION_USUARIO:
    {
        funcion_bcode = (lat_bytecode *)__memoria_asignar(sizeof(lat_bytecode) * MAX_BYTECODE_FUNCTION);
        fi = 0;
        // procesar lista de argumentos
        if (node->izq)
        {
            fpn(vm, node->izq);
        }
        // procesar instrucciones
        fpn(vm, node->der);        
        funcion_bcode = __memoria_reasignar(funcion_bcode, sizeof(lat_bytecode) * (fi+1));
        dbc(MAKE_FUNCTION, 0, 0, (void*)funcion_bcode);
        funcion_bcode = NULL;
        fi = 0;
    }
    break;
    
    case NODO_LISTA:
    {
        /*lat_objeto* ret = lat_decimal_nuevo(vm, 0);
        dbc(OP_STOREDOUBLE, 0, 0, ret);
#if DEPURAR_AST
            printf("STOREINT R0\t%.14g\n", ret->data.d );
#endif
        if (node->izq)
        {
            pn(vm, node->izq);
        }
        dbc(OP_STORELIST, 255, 0, NULL);
#if DEPURAR_AST
            printf("STORELIST R255\n");
#endif*/
    }
    break;
    case NODO_LISTA_AGREGAR_ELEMENTO:
    {
        /*if (node->der)
        {
            pn(vm, node->der);
        }
        if (node->izq)
        {
            pn(vm, node->izq);
            dbc(OP_PUSH, 255, 0, NULL);
            dbc(OP_INC, 0, 0, NULL);
#if DEPURAR_AST
            printf("PUSH R255\n");
            printf("INC R0\n");
#endif
        }*/
    }
    break;
    case NODO_LISTA_ASIGNAR_ELEMENTO:
    {
        /*nodo_lista_elem *elem = ((nodo_lista_elem *)node);
        if (elem->exp)
        {
            pn(vm, elem->exp);
            dbc(OP_MOV, 3, 255, NULL);
#if DEPURAR_AST
            printf("MOV R3 R255\n");
#endif
        }
        if (elem->id)
        {
            pn(vm, elem->pos);
            dbc(OP_MOV, 4, 255, NULL);
#if DEPURAR_AST
            printf("MOV R4 R255\n");
#endif
            pn(vm, elem->id);
            dbc(OP_LISTSETITEM, 255, 3, (void *)4);
#if DEPURAR_AST
            printf("LISTSETITEM R255 R3\n");
#endif
        }*/
    }
    break;
    case NODO_LISTA_OBTENER_ELEMENTO:
    {
        /*if (node->izq)
        {
            pn(vm, node->izq);
            dbc(OP_MOV, 3, 255, NULL);
#if DEPURAR_AST
            printf("MOV R3 R255\n");
#endif
        }
        if (node->der)
        {
            pn(vm, node->der);
            dbc(OP_LISTGETITEM, 255, 3, NULL);
            dbc(OP_PUSH, 255, 0, NULL);
#if DEPURAR_AST
            printf("LISTGETITEM R55 R3\n");
            printf("PUSH R55\n");
#endif
        }*/
    }
    break;
    case NODO_DICCIONARIO:
    {
        /*nested++;
        dbc(OP_STOREDICT, nested, 0, NULL);
#if DEPURAR_AST
        printf("STOREDICT R%i\n", nested);
#endif
        if (node->izq)
        {
            pn(vm, node->izq);
            dbc(OP_MOV, 255, nested, NULL);
#if DEPURAR_AST
            printf("MOV R255 R%i\n", nested);
#endif
        }
        nested--;*/
    }
    break;
    case NODO_DICC_AGREGAR_ELEMENTO:
    {
        /*if (node->izq)
        {
            pn(vm, node->izq);
            dbc(OP_PUSHDICT, nested, 255, NULL);
#if DEPURAR_AST
            printf("PUSHDICT R%i R255\n", nested);
#endif
        }
        if (node->der)
        {
            pn(vm, node->der);
        }*/
    }
    break;
    case NODO_DICC_ELEMENTO:
    {
        /*if (node->izq)
        {
            pn(vm, node->izq);
            dbc(OP_PUSH, 255, 0, NULL);
#if DEPURAR_AST
            printf("PUSH R255\n");
#endif
        }
        if (node->der)
        {
            pn(vm, node->der);
            dbc(OP_PUSH, 255, 0, NULL);
#if DEPURAR_AST
            printf("PUSH R255\n");
#endif
        }
        dbc(OP_PUSHDICTELEM, 0, 0, NULL);
#if DEPURAR_AST
        printf("OP_PUSHDICTELEM\n");
#endif*/
    }
    break;
    case NODO_DICC_ASIGNAR_ELEMENTO:
    {
        /*nodo_dicc_elem *elem = ((nodo_dicc_elem *)node);
        if (elem->exp)
        {
            pn(vm, elem->exp);
            dbc(OP_MOV, 3, 255, NULL);
#if DEPURAR_AST
            printf("MOV R3 R255\n");
#endif
        }
        if (elem->id)
        {
            pn(vm, elem->llave);
            dbc(OP_MOV, 4, 255, NULL);
#if DEPURAR_AST
            printf("MOV R4 R255\n");
#endif
            pn(vm, elem->id);
            dbc(OP_DICTSETITEM, 255, 3, (void*)4);
#if DEPURAR_AST
            printf("DICTITEM R255 R3\n");
#endif
        }*/
    }
    break;
    case NODO_DICC_OBTENER_ELEMENTO:
    {
        /*if (node->izq)
        {
            pn(vm, node->izq);
            dbc(OP_MOV, 3, 255, NULL);
#if DEPURAR_AST
            printf("MOV R3 R255\n");
#endif
        }
        if (node->der)
        {
            pn(vm, node->der);
            dbc(OP_DICTGETITEM, 255, 3, NULL);
#if DEPURAR_AST
            printf("DICTGETITEM R255 R3\n");
#endif
        }*/
    }
    break;
    default:
        printf("nodo_tipo:%i\n", node->tipo);
        return 0;
    }
    // printf("i = %i\n", i);
    return i;
}


static void __mostrar_bytecode(lat_bytecode *bcode){
    lat_bytecode *inslist = bcode;
    lat_bytecode cur;
    lat_objeto *o;
    int pos;

    for (pos = 0, cur = inslist[pos]; cur.ins != NULL && cur.ins != HALT; cur = inslist[++pos])
    {
        printf("%i\t", pos);
CONTINUAR:
        switch (cur.ins)
        {
            case HALT:
                return;
                break;
            case LOAD_CONST:{
                o = (lat_objeto*)cur.meta;
                printf("LOAD_CONST\t(%s)\n", __objeto_a_cadena(o));
            } break;
            case LOAD_NAME: {
                o = (lat_objeto*)cur.meta;
                printf("LOAD_NAME \t(%s)\n", __objeto_a_cadena(o));
            } break;
            case STORE_NAME: {
                o = (lat_objeto*)cur.meta;
                printf("STORE_NAME\t(%s)\n", __objeto_a_cadena(o));
            } break;
            case NOP:            
            case UNARY_MINUS:
            case BINARY_ADD:
            case BINARY_SUB:
            case BINARY_MUL:
            case BINARY_DIV:
            case BINARY_MOD:
            case OP_GT:
            case OP_GE:
            case OP_LT:
            case OP_LE:
            case OP_EQ:
            case OP_NEQ:
            case OP_AND:
            case OP_OR:
            case OP_NOT:
            case OP_INC:
            case OP_DEC:
            case CONCAT:
            case SETUP_LOOP:
            case POP_BLOCK:
            case CALL_FUNCTION:                        
                printf("%s\n", __obtener_bytecode_nombre(cur.ins));
            break;
            case RETURN_VALUE:
                printf("%s\n", __obtener_bytecode_nombre(cur.ins));                
                //return;
            break;
            case JUMP_ABSOLUTE:{
                printf("JUMP_ABSOLUTE\t(%i)\n", (cur.a+1));
            }break;
            case POP_JUMP_IF_FALSE:{
                printf("POP_JUMP_IF_FALSE\t(%i)\n", (cur.a+1));
            }break;
            case POP_JUMP_IF_TRUE:{
                printf("POP_JUMP_IF_TRUE\t(%i)\n", (cur.a+1));
            }break;
            case MAKE_FUNCTION:{
                printf("MAKE_FUNCTION\n");
                printf("-------------------------------\n");
                //lat_bytecode *bcode = (lat_bytecode*)cur.meta;                
                __mostrar_bytecode(cur.meta);                
                printf("-------------------------------\n");
                cur.ins++;
                goto CONTINUAR;                
            }break;
        }
    }
}

lat_objeto *nodo_analizar_arbol(lat_vm *vm, ast *tree)
{
    lat_bytecode *bcode = (lat_bytecode *)__memoria_asignar(sizeof(lat_bytecode) * MAX_BYTECODE_FUNCTION);
    int i = nodo_analizar(vm, tree, bcode, 0);
    dbc(HALT, 0, 0, NULL);
    __memoria_reasignar(bcode, sizeof(lat_bytecode) * (i+1));
#if DEPURAR_AST
    __mostrar_bytecode(bcode);
#endif
    return lat_definir_funcion(vm, bcode);
}
