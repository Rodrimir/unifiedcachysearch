#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
shopt -s dotglob
files=(*)
shopt -u dotglob
tar czf ./unifiedcachysearch.tar.gz \
    --transform 's|^|unifiedcachysearch/|' \
    --exclude='build' \
    --exclude='.git' \
    --exclude='logs' \
    --exclude='unifiedcachysearch.tar.gz' \
    --exclude='*.tar.gz' \
    "${files[@]}"
echo "Criado: $(realpath ./unifiedcachysearch.tar.gz)"
