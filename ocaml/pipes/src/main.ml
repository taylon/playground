let write_pid input_fd =
  try
    let pid = Bytes.of_string ("(" ^ string_of_int (Unix.getpid ()) ^ ")") in
    ignore (Unix.write input_fd pid 0 (Bytes.length pid));

    Unix.close input_fd
  with Unix.Unix_error (n, f, arg) ->
    Printf.printf "%s(%s) : %s\n" f arg (Unix.error_message n)

let output_fd, input_fd = Unix.pipe ()

;;
match Unix.fork () with
| 0 ->
  for _i = 0 to 5 do
    match Unix.fork () with
    | 0 ->
      write_pid input_fd;
      exit 0
    | _ -> ()
  done;

  Unix.close input_fd
| _ ->
  Unix.close input_fd;
  let pids = ref ""
  and buff = Bytes.create 5 in

  while true do
    match Unix.read output_fd buff 0 5 with
    | 0 ->
      Printf.printf "Children PIDs %s\n" !pids;
      exit 0
    | n -> pids := !pids ^ String.sub (Bytes.to_string buff) 0 n ^ "."
  done
