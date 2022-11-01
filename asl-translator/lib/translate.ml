open Llvm
open LibASL.Asl_ast
open LibASL.Asl_utils

let a = 2

let ctx : llcontext = Llvm.global_context ()

let llmodule : llmodule = Llvm_irreader.parse_ir ctx (Llvm.MemoryBuffer.of_file "./lib/state.ll")

let void_t = void_type ctx
let void_fun_t : lltype = function_type void_t [| |]

let func : llvalue = declare_function "main" void_fun_t llmodule
(* let entry = entry_block func *)

let translate_prim (nm: string) (tes: expr list) (es: expr list) : llvalue = 
  match nm,tes,es with 
  | _ -> assert false

let translate_ty (ty: ty) : lltype = 
  match ty with 
  | Type_Bits (Expr_LitInt n) -> integer_type ctx (int_of_string n)
  | Type_Bits _
  | Type_Constructor _
  | Type_OfExpr _
  | Type_Register (_, _)
  | Type_Array (_, _)
  | Type_Tuple _
  | Type_App _ -> failwith @@ "translate_ty: " ^ pp_type ty

let ident = function | Ident x | FIdent (x,_) -> x

let rec translate_expr (exp : expr) : llvalue = 
  match exp with 
  | Expr_Field (_, _) -> undef (i1_type ctx)
  | Expr_Slices (_, _) -> undef (i1_type ctx)
  | Expr_In (_, _) -> assert false
  | Expr_Var _ -> assert false
  | Expr_Array (_, _) -> undef (i1_type ctx)
  | Expr_Parens e -> translate_expr e
  | Expr_TApply (nm, tes, es) -> translate_prim (ident nm) tes es
  
  | Expr_LitInt _ -> assert false
  | Expr_LitHex _ -> assert false
  | Expr_LitBits _ -> assert false
  
  | Expr_LitReal _ 
  | Expr_LitMask _ 
  | Expr_LitString _ 
  
  | Expr_If (_, _, _, _, _)
  | Expr_Fields (_, _)  
  | Expr_Tuple _        
  | Expr_Binop (_, _, _)
  | Expr_Unop (_, _)    
  | Expr_ImpDef (_, _)  
  | Expr_Unknown _      
  -> failwith @@ "unhandled translate_expr: " ^ pp_expr exp

(** a tuple of entry and exit basic blocks. 

    has the following properties: 
    - the two blocks are linked, with the entry block 
      suitable for entering the concept described
    - the exit block does not have a terminating instruction
    - the two blocks may refer to the same block if there is only one.
 *)
type blockpair = llbasicblock * llbasicblock

let translate_stmt (stmt : LibASL.Asl_ast.stmt) : llbasicblock * llbasicblock = 
  let block = append_block ctx "" func in
  let build = builder_at_end ctx block in
  let single = (block, block) in

  match stmt with 
  | Stmt_Assert _ -> single
  | Stmt_VarDeclsNoInit (_ty, _nms, _) -> single
  | Stmt_VarDecl (ty, nm, rv, _)  
  | Stmt_ConstDecl (ty, nm, rv, _) ->
    let ty = translate_ty ty in
    let alloc = build_alloca ty (ident nm) build in
    let _ = build_store (translate_expr rv) alloc build in
    single
  | Stmt_Assign (_, _, _) -> single
  | Stmt_Throw (_, _) -> assert false
  | Stmt_TCall (_, _, _, _) -> assert false
  | Stmt_If (_, _, _, _, _) -> assert false
  
  | Stmt_Unpred _ 
  | Stmt_ConstrainedUnpred _ 
  | Stmt_ImpDef (_, _) 
  | Stmt_Undefined _ 
  | Stmt_ExceptionTaken _ 
  | Stmt_See (_, _) 
  | Stmt_Case (_, _, _, _) 
  | Stmt_Dep_Unpred _ 
  | Stmt_Dep_ImpDef (_, _) 
  | Stmt_Dep_Undefined _ 
  | Stmt_DecodeExecute (_, _, _) 
  | Stmt_For (_, _, _, _, _, _) 
  | Stmt_While (_, _, _) 
  | Stmt_Repeat (_, _, _) 
  | Stmt_Try (_, _, _, _, _) 
  | Stmt_FunReturn (_, _) 
  | Stmt_ProcReturn _ 
  -> failwith @@ "unhandled translate_stmt: " ^ pp_stmt stmt

let rec translate_stmts (stmts : stmt list) : llbasicblock * llbasicblock = 
  match stmts with 
  | [] -> 
    let b = append_block ctx "" func in b,b
  | s::rest -> 
    let (b1, b2) = translate_stmt s in
    let (rest1, rest2) = translate_stmts rest in
    
    let builder = builder_at_end ctx b2 in
    ignore @@ build_br rest1 builder;
    (b1, rest2)

  
let load_aslb f = 
  let chan = open_in_bin f in 
  let stmts : LibASL.Asl_ast.stmt list = Marshal.from_channel chan in
  (* List.iter (fun x -> print_endline (LibASL.Asl_utils.pp_stmt x)) stmts *)
  stmts
