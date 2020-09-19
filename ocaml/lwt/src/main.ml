(* let get_user_input = Lwt_io.read_line Lwt_io.stdin *)

(* let value_typed =   *)
(*   Lwt.bind get_user_input (fun value -> Lwt_io.printlf "You typed %S" value) *)

(* let () = Lwt_main.run value_typed *)

let get_user_input = Lwt_io.read_line Lwt_io.stdin 


let%lwt value_typed = get_user_input in 
  Lwt_io.print "You typed %S"


(* let value_typed =   *)
(*   Lwt.bind get_user_input (fun value -> Lwt_io.printlf "You typed %S" value) *)

let () = Lwt_main.run value_typed

