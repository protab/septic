#!/bin/bash
server_out=.test.server.out
failures=.test.failures

tmpd=$(mktemp -d)
trap 'rm -rf $tmpd' EXIT

rm -f $server_out $failures

function parse_client()
{
	while IFS= read -r line; do
		[[ $line == "* "* ]] && continue
		if [[ $line == "> "* ]]; then
			line=${line#> }
			[[ $line == TestFailure* ]] && echo "$line" >> $failures
			echo "$line"
			continue
		fi
		echo "??? $line"
	done
}

function check_server()
{
	tb=$(tail -n +$((prev_loglen + 1)) $server_out | grep -n -m 1 '^Traceback')
	if [[ -n $tb ]]; then
		lineno=${tb%%:*}
		lineno=$((lineno + prev_loglen))
		echo "Server fail at $server_out:$lineno" >> $failures
		echo "Server fail at $server_out:$lineno"
	fi
	prev_loglen=$(wc -l < $server_out)
}

PYTHONPATH=$PWD/tests ./septic > $server_out 2>&1 &
sleep 0.5
prev_loglen=0
for t in tests/*_c.py; do
	m=${t##*/}
	m=${m%_c.py}
	echo "Test case $m"
	m=${m}_s
	cat tests/testlib.py $t > $tmpd/client.py
	./client -b -m $m -p $tmpd/client.py | parse_client
	check_server
done
kill %1

if [[ -f $failures ]]; then
	cnt=$(wc -l < $failures)
	echo "$cnt tests FAILED"
	exit 1
fi
echo "Everything OK."
exit 0
