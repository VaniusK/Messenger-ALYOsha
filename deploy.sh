#!/bin/bash
set -e

IMAGE="c_project-server:latest"
REMOTE="ivan@158.160.219.207"
REMOTE_DIR="~/"

echo "Building..."
make run_docker

echo "Saving..."
docker save $IMAGE | gzip > /tmp/alesha.gz

echo "Uploading..."
scp /tmp/alesha.gz $REMOTE:$REMOTE_DIR/

echo "Loading and running on VM..."
ssh -i ~/.ssh/my_key.pem $REMOTE << 'EOF'
  cp alesha.gz Messenger-ALYOsha/alesha.gz
  cd Messenger-ALYOsha
  docker load -i alesha.gz
  docker tag c_project-server:latest messenger-alyosha-server:latest
  docker compose --profile prod up -d
EOF

echo "Done!"