(* let () = Test.Translate.f () *)

let () = 
  let open Test.Translate in
  let stmts = load_aslb Sys.argv.(1) in 
  print_newline ();
  List.iter (fun x -> x |> LibASL.Asl_utils.pp_stmt |> print_endline) stmts;
  print_newline ();
  ignore @@ translate_stmts stmts;
  Llvm.dump_module llmodule
