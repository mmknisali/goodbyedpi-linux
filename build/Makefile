# GoodbyeDPI Linux - Docker Makefile
# Convenient commands for managing the Docker container

.PHONY: help build up down restart logs shell clean rebuild test

# Default target
help:
	@echo "GoodbyeDPI Linux - Docker Commands"
	@echo ""
	@echo "Usage: make [target]"
	@echo ""
	@echo "Targets:"
	@echo "  build       Build the Docker image"
	@echo "  up          Start the container in background"
	@echo "  down        Stop and remove the container"
	@echo "  restart     Restart the container"
	@echo "  logs        Show container logs (live)"
	@echo "  shell       Open shell in running container"
	@echo "  clean       Remove container and volumes"
	@echo "  rebuild     Clean rebuild (no cache)"
	@echo "  test        Run with debug mode for testing"
	@echo "  status      Show container status"
	@echo ""
	@echo "Examples:"
	@echo "  make build      # Build the image"
	@echo "  make up         # Start GoodbyeDPI"
	@echo "  make logs       # Watch logs"
	@echo "  make down       # Stop GoodbyeDPI"

# Build the Docker image
build:
	@echo "Building GoodbyeDPI Docker image..."
	docker-compose build

# Start container in background
up:
	@echo "Starting GoodbyeDPI container..."
	docker-compose up -d
	@echo "Container started! Use 'make logs' to view logs"

# Stop and remove container
down:
	@echo "Stopping GoodbyeDPI container..."
	docker-compose down
	@echo "Container stopped and removed"

# Restart container
restart:
	@echo "Restarting GoodbyeDPI container..."
	docker-compose restart

# Show logs (follow mode)
logs:
	@echo "Showing GoodbyeDPI logs (Ctrl+C to exit)..."
	docker-compose logs -f

# Open shell in running container
shell:
	@echo "Opening shell in container..."
	docker-compose exec goodbyedpi /bin/bash

# Clean everything (including volumes)
clean:
	@echo "Cleaning up containers and volumes..."
	docker-compose down -v
	@echo "Cleanup complete"

# Rebuild from scratch (no cache)
rebuild:
	@echo "Rebuilding from scratch (no cache)..."
	docker-compose build --no-cache
	@echo "Rebuild complete"

# Run in test/debug mode
test:
	@echo "Running in debug mode..."
	docker-compose run --rm goodbyedpi --debug -v -9

# Show container status
status:
	@echo "Container status:"
	@docker-compose ps
	@echo ""
	@echo "Resource usage:"
	@docker stats goodbyedpi --no-stream 2>/dev/null || echo "Container not running"

# Check iptables rules (on host)
check-iptables:
	@echo "Checking iptables rules..."
	@sudo iptables -L -n -v | grep NFQUEUE || echo "No NFQUEUE rules found"

# Quick start (build and run)
quickstart: build up logs

# Full reset (stop, clean, rebuild, start)
reset: down clean rebuild up
	@echo "Full reset complete!"
