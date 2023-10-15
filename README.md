# samplecode

## mempool_redis_http : 메모리 풀/레디스 요청/HTTP 요청
- httprequest.hpp : HTTP 요청(get/post)
- mempool : 특정 크기의 메모리 풀. 크기마다 풀링
- redis : get/set, hmget/hmset 래핑

## network : 서버/세션
- server.hpp : simple 서버
- session : 연결 관리

## sql_load_query :
- rowdata.h : 한 row 를 나타냄
- table.h : 데이터는 row 의 집합. 테이블을 나타냄
- metadb.h : sql DB 파일이 갱신될 때 다시 로딩. 하나의 DB 를 표시
- query.h/cpp : metadb 에 쿼리를 실행할 수 있도록 만든 유틸
- loader.h : sql 파일을 로딩하는 객체. 
