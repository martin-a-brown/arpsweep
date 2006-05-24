#! /bin/bash
#
# -- example tcpdump output

# -- my favorite shell functions (and their offspring)
#
gripe  () { printf "%s\n" "$@" >&2; }
abort  () { gripe "$@"; exit 1;     }

pcap_to_stdout () {

  mimencode -u <<-EOPCAP
	1MOyoQIABAAAAAAAAAAAAP//AAABAAAAmpBNRGzhBQAqAAAAKgAAAACAyPt42AAwG694UQgG
	AAEIAAYEAAEAMBuveFEKChQsAAAAAAAACgoUAZqQTUTY4QUAPAAAADwAAAAAMBuveFEAgMj7
	eNgIBgABCAAGBAACAIDI+3jYCgoUAQAwG694UQoKFCyU+YGAAAEAAAAAAAAKZGFpbWE=
EOPCAP
 :;
}
guess_tcpdump () {
  which tcpdump 2>/dev/null
  echo /usr/{sbin,local/bin}/tcpdump;
}

for tcpdump in $( guess_tcpdump ) ; do

  test -x $tcpdump   || abort "Could not find/run tcpdump to show PCAP."

  pcap_to_stdout | $tcpdump 2>/dev/null -entxxs0  -r /dev/stdin && exit

done

