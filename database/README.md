# Poster Safari -- Database

Poster Safari uses couchdb 2.0

The official dockerfile is on a lower version, we use klaemo/couchdb instead.

## Setup

1. install docker
 1. create file `Dockerfile` with content

    ```
    FROM klaemo/couchdb:2.0.0
    ```
 
 1. create file `docker-compse.yml`
 
    ``` 
    version: '2'
    services:
        fallstudie:
            build: .
            restart: always
            ports:
                - "5984:5984"
            environment:
                - COUCHDB_USER=REPLACEWITHADMINNAME
                - COUCHDB_PASSWORD=REPLACEWITHADMINPASSWORD
            volumes:
                - /srv/docker/couchdb:/usr/local/var/lib/couchdb
    ```
 
 1. run couchdb with `docker-compose up -d`
