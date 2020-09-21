open Unix

let sock =
  let fd = socket PF_UNIX SOCK_DGRAM 0 in
  bind fd (ADDR_UNIX "server.sock");
  fd

;;
let buff = Bytes.create 8192 in
let size, _ = recvfrom sock buff 0 8192 [] in
Printf.printf "received: %s" (String.sub (Bytes.to_string buff) 0 size)

;;
close sock
