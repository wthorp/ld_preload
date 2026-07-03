APP_NAME := ld_preload
SRC := .
SO_NAME := ld_preload.so
GOFMT_FILES := $(shell find . -name '*.go' -not -path './vendor/*')
C_FILES := $(shell find . \( -name '*.c' -o -name '*.h' \) -not -path './vendor/*')
GOLANGCI_LINT_VERSION := v2.4.0
STATICCHECK_VERSION := 2025.1.1
LEFTHOOK_VERSION := v1.12.4
GOBIN ?= $(or $(shell go env GOBIN),$(firstword $(subst :, ,$(shell go env GOPATH)))/bin)
GOLANGCI_LINT := $(GOBIN)/golangci-lint
STATICCHECK := $(GOBIN)/staticcheck
LEFTHOOK := $(GOBIN)/lefthook
SMOKE_IMAGE_BOOKWORM := golang:1.26-bookworm
SMOKE_IMAGE_BULLSEYE := golang:1.24-bullseye
SMOKE_TAG_BOOKWORM := ld_preload-smoke:bookworm
SMOKE_TAG_BULLSEYE := ld_preload-smoke:bullseye

.DEFAULT_GOAL := help

.PHONY: help
help:
	@printf "Usage:\n"
	@printf "  make %-12s %s\n" "build" "Build the executable and shared object"
	@printf "  make %-12s %s\n" "fmt" "Format Go and C sources"
	@printf "  make %-12s %s\n" "fmt-check" "Verify formatting without changing files"
	@printf "  make %-12s %s\n" "lint" "Run Go linters and tests"
	@printf "  make %-12s %s\n" "lint-fix" "Apply supported auto-fixes"
	@printf "  make %-12s %s\n" "docker-smoke" "Run Linux LD_PRELOAD smoke tests in both Docker images"
	@printf "  make %-12s %s\n" "docker-smoke-bookworm" "Run smoke tests in Debian bookworm"
	@printf "  make %-12s %s\n" "docker-smoke-bullseye" "Run smoke tests in Debian bullseye"
	@printf "  make %-12s %s\n" "hooks" "Install local git hooks with lefthook"
	@printf "  make %-12s %s\n" "tools" "Install Go-based developer tools"
	@printf "  make %-12s %s\n" "clean" "Remove build artifacts"

.PHONY: all
all: build

.PHONY: build
build:
	go build -o $(APP_NAME) $(SRC)
	@if [ "$$(uname -s)" = "Linux" ]; then \
		go build -o $(SO_NAME) -buildmode=c-shared $(SRC); \
	else \
		printf '%s\n' 'Skipping shared object build on non-Linux host.'; \
	fi

.PHONY: run
run: build
	./$(APP_NAME)

.PHONY: tools
tools:
	go install github.com/golangci/golangci-lint/v2/cmd/golangci-lint@$(GOLANGCI_LINT_VERSION)
	go install honnef.co/go/tools/cmd/staticcheck@$(STATICCHECK_VERSION)
	go install github.com/evilmartians/lefthook@$(LEFTHOOK_VERSION)

.PHONY: hooks
hooks:
	$(LEFTHOOK) install

.PHONY: fmt
fmt:
	gofmt -w $(GOFMT_FILES)
	@if command -v clang-format >/dev/null 2>&1; then \
		clang-format -i $(C_FILES); \
	else \
		printf '%s\n' 'clang-format not found; Go files formatted, C files skipped.'; \
	fi

.PHONY: fmt-check
fmt-check:
	@test -z "$$(gofmt -l $(GOFMT_FILES))" || \
		(printf '%s\n' 'Go files need formatting:'; gofmt -l $(GOFMT_FILES); exit 1)
	@if command -v clang-format >/dev/null 2>&1; then \
		tmpdir=$$(mktemp -d); \
		failed=0; \
		for file in $(C_FILES); do \
			out="$$tmpdir/$$(basename "$$file")"; \
			clang-format "$$file" > "$$out"; \
			if ! cmp -s "$$file" "$$out"; then \
				printf '%s\n' "$$file"; \
				failed=1; \
			fi; \
		done; \
		rm -rf "$$tmpdir"; \
		test $$failed -eq 0 || (printf '%s\n' 'C files need formatting.'; exit 1); \
	else \
		printf '%s\n' 'clang-format not found; skipping C format check.'; \
	fi

.PHONY: lint
lint:
	$(GOLANGCI_LINT) run
	$(STATICCHECK) ./...
	go test ./...

.PHONY: lint-fix
lint-fix:
	$(GOLANGCI_LINT) run --fix
	$(MAKE) fmt

.PHONY: docker-smoke
docker-smoke: docker-smoke-bookworm docker-smoke-bullseye

.PHONY: docker-smoke-bookworm
docker-smoke-bookworm:
	chmod +x smoke/run.sh
	docker build --build-arg GO_BASE_IMAGE=$(SMOKE_IMAGE_BOOKWORM) -f Dockerfile.smoke -t $(SMOKE_TAG_BOOKWORM) .
	mkdir -p smoke-artifacts/bookworm
	docker run --rm -e SMOKE_SUITE=bookworm -v "$(CURDIR)/smoke-artifacts:/workspace/smoke-artifacts" $(SMOKE_TAG_BOOKWORM)

.PHONY: docker-smoke-bullseye
docker-smoke-bullseye:
	chmod +x smoke/run.sh
	docker build --build-arg GO_BASE_IMAGE=$(SMOKE_IMAGE_BULLSEYE) -f Dockerfile.smoke -t $(SMOKE_TAG_BULLSEYE) .
	mkdir -p smoke-artifacts/bullseye
	docker run --rm -e SMOKE_SUITE=bullseye -v "$(CURDIR)/smoke-artifacts:/workspace/smoke-artifacts" $(SMOKE_TAG_BULLSEYE)

.PHONY: clean
clean:
	rm -f $(APP_NAME) $(SO_NAME) ld_preload.h
