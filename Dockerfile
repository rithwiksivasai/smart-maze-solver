FROM gcc:14-bookworm AS build
WORKDIR /src
COPY backend/main.cpp ./main.cpp
RUN g++ -std=c++17 -O2 -Wall -Wextra -o /maze_server main.cpp

FROM debian:bookworm-slim
RUN apt-get update \
    && apt-get install -y --no-install-recommends libstdc++6 \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY --from=build /maze_server ./maze_server
COPY web ./web
ENV PORT=8080
EXPOSE 8080
CMD ["./maze_server"]
