open Unix

let sock =
  let fd = socket PF_UNIX SOCK_DGRAM 0 in
  connect fd (ADDR_UNIX "server.sock");
  fd

let send message =
  let bytes = Bytes.of_string message in
  send sock bytes 0 (Bytes.length bytes) []

;;
send "hello!"

;;
close sock
