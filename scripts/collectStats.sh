while [ 1 -eq 1 ]; do
  docker stats --no-stream --format "{{.CPUPerc}} | {{.NetIO}}" ep1_ep1_1 >> results/$1/output.txt
done