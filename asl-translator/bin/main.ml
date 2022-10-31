(* let () = Test.Translate.f () *)

let () = 
  let open Test.Translate in
  let stmts = load_aslb Sys.argv.(1) in 
  ignore @@ translate_stmts stmts
