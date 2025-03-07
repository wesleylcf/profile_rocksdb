echo "Current ulimit:"
ulimit -Sn
ulimit -Hn

ulimit -n 66536

echo "Increased ulimit:"
ulimit -Sn
ulimit -Hn

echo "Compiling generate_data:"
cd model && make

echo "Executing generate_data:"
cd .. && ./generate_data