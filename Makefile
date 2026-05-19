CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra


format:
	find server client common  -path "*/build/*" -prune -o  -path "server/drogon" -prune -o \( -name "*.hpp" -o -name "*.cpp" \) -print 2>/dev/null| xargs -r clang-format-17 -i

build_client:
	docker compose --profile build run --build client-build

run_server_tests:
	docker compose -f 'docker-compose.yml' up --build 'test-server' 

run_server:
	docker compose -f 'docker-compose.yml' up -d --build 'nginx' 