(* let () = Test.Translate.f () *)

let () = 
  let open Test.Translate in
  let fname = Sys.argv.(1) in
  let stmts = load_aslb fname in 
  List.iter (fun x -> x |> LibASL.Asl_utils.pp_stmt |> Printf.eprintf "%s\n") stmts;
  ignore @@ translate_stmts_entry stmts;
  
  Llvm.print_module "/dev/stdout" llmodule;
  match Llvm_analysis.verify_module llmodule with 
  | None -> () 
  | Some s -> failwith @@ "verify_module failed: " ^ s
