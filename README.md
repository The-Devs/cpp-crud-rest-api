# C++ CRUD REST API
Uma API REST desenvolvida em C++ para serviço de CRUD (Create, Read, Update, Delete) em banco de dados.

## Comando e argumentos
Antes de iniciar o servidor CRUD REST API (CRA) devemos configurar um arquivo `.json` com informações do banco de dados:

~dir/meu_db.json
```json
{
"name":"theadmin",
"username":"usuario_db",
"password":"senha do usuario",
"host":"127.0.0.1",
"protocol":"tcp",
"port":"3306",
}
```

Para iniciar o servidor devemos executar o binário (nome padrão cpp-cra):
```sh
$ cpp-cra localhost 8100 ~dir/meu_db.json
```
Uma mensagem de inicialização será enviada para o console.

Para parar o servidor devemos realizar uma requisição `DELETE` para a rota `"/"`. Exemplo via curl:
```sh
$ curl -X DELETE http://localhost:8100
```
Uma mensagem de encerramento será enviada para o console.

Atualmente C++ CRA suporta apenas arquivos `.json` **sem espaços ou tabulações**. Veja os tópicos 2 e 3 em "Implementações em planejamento" para mais detalhes.

## Banco de dados suportados
* MySQL

## Colaboradores
Pessoas que estiveram presente no desenvolvimento do projeto:
* [enriquerene](https://github.com/enriquerene) - Planejamento e desenvolvimento.

## Third-party
Uma lista de bibliotecas que possibilitaram o desenvolvimento deste projeto da maneira como é hoje:
* [yhirose/cpp-httplib](https://github.com/yhirose/cpp-httplib)

## Implementações em planejamento
1. Middleware:
    * Privacidade de rotas com usuário básico.
    * Privacidade de rotas com token.
2. Lidando com arquivos:
    * ~~Ler arquivo json e armazenar em map.~~
    * ~~Ler map armazenado.~~
    * Suporte a json incluindo arrays.
    * Suporte a json incluindo objetos.
    * Suporte a formato TXT.
    * Suporte a formato XML.
3. Ampliar ferramentas via rotas:
    * ~~Filtro de leitura com parâmetros query na URL.~~
    * Aplicar Trim em entradas json para processos de "sqlização".
4. Suporte a outros bancos de dados:
    * PostgrSQL.
    * MongoDB.
5. Suporte a outros formatos de corpo na resposta:
    * HTML
    * CSS
    * JS
    * XML