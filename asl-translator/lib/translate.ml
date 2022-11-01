open Llvm
open LibASL.Asl_ast
open LibASL.Asl_utils

let a = 2

let ctx : llcontext = Llvm.global_context ()

let llmodule : llmodule = Llvm_irreader.parse_ir ctx (Llvm.MemoryBuffer.of_file "./lib/state.ll")

let bool_t = i1_type ctx
let bool_true = const_all_ones bool_t 
let bool_false = const_null bool_t
let void_t = void_type ctx
let void_fun_t : lltype = function_type void_t [| |]

let func : llvalue = define_function "main" void_fun_t llmodule
(* let entry = entry_block func *)

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

module Globals = Map.Make(String)
let vars : llvalue Globals.t ref = ref Globals.empty


let unknown_loc = LibASL.Asl_ast.Unknown
let lexpr_to_expr = LibASL.Symbolic.lexpr_to_expr unknown_loc
let expr_to_lexpr = LibASL.Symbolic.expr_to_lexpr


let rec translate_prim (nm: string) (tes: expr list) (es: expr list) (build: llbuilder): llvalue = 
  let err s = failwith @@ "translate_prim: " ^ s ^ " at " ^ pp_expr (Expr_TApply (Ident nm, tes, es)) in
  match nm,tes,es with 
  | "add_bits",_,[x;y] -> 
    let x = translate_expr x build and y = translate_expr y build in 
    build_add x y "" build

  | "eq_bits",_,[x;y] -> 
    let x = translate_expr x build and y = translate_expr y build in 
    build_icmp Icmp.Eq x y "" build

  | "ZeroExtend",_,[x;Expr_LitInt n] -> 
    let x = translate_expr x build and n = int_of_string n in
    build_zext x (integer_type ctx n) "" build
  | "SignExtend",_,[x;Expr_LitInt n] -> 
    let x = translate_expr x build and n = int_of_string n in
    build_sext x (integer_type ctx n) "" build

  | _ -> err "unsupported"


and translate_expr (exp : expr) (build: llbuilder) : llvalue = 
  let err s = failwith @@ "translate_expr: " ^ s ^ " at " ^ pp_expr exp in
  match exp with 
  | Expr_Slices (base, [Slice_LoWd (Expr_LitInt lo,Expr_LitInt wd)]) ->    
    let base = translate_expr base build in
    let lo = int_of_string lo and wd = int_of_string wd in
    let ty = type_of base in
    let ty' = integer_type ctx wd in
    
    let lshr = 
      if lo = 0 
        then base 
        else build_lshr base (const_int ty lo) "" build
    in
    if integer_bitwidth ty' = integer_bitwidth ty
      then lshr 
      else build_trunc lshr ty' "" build
  
  | Expr_Var (Ident "TRUE") -> bool_true
  | Expr_Var (Ident "FALSE") -> bool_false
  
  | Expr_Var _ 
  | Expr_Array _ 
  | Expr_Field (_, _) -> 
    let ptr = translate_lexpr (expr_to_lexpr exp) in
    build_load ptr "" build 

  | Expr_Parens e -> translate_expr e build
  | Expr_TApply (nm, tes, es) -> translate_prim (ident nm) tes es build
  
  | Expr_LitInt _ -> assert false
  | Expr_LitHex _ -> assert false
  | Expr_LitBits bits -> 
    let ty = integer_type ctx (String.length bits) in
    const_int ty (int_of_string ("0b" ^ bits))
    
  | Expr_LitReal _ 
  | Expr_LitMask _ 
  | Expr_LitString _ 
      
  | Expr_Slices _
  | Expr_In (_, _) 
  | Expr_If (_, _, _, _, _)
  | Expr_Fields (_, _)  
  | Expr_Tuple _        
  | Expr_Binop (_, _, _)
  | Expr_Unop (_, _)    
  | Expr_ImpDef (_, _)  
  | Expr_Unknown _      
  -> err "unsupported expression"


and translate_lexpr (lexp: lexpr) : llvalue = 
  let err s = failwith @@ "translate_lexpr: " ^ s ^ " at " ^ pp_lexpr lexp in
  let find nm = 
    try Globals.find nm !vars 
    with Not_found -> err @@ "cannot find variable " ^ nm
  in

  match lexp with 

  | LExpr_Var (Ident "_PC") -> find "PC"
  | LExpr_Var nm -> find (ident nm)
  
  | LExpr_Array (LExpr_Var (Ident "_R"), Expr_LitInt n) -> find ("X"^n)
  | LExpr_Field (LExpr_Var (Ident "PSTATE"), Ident "nRW") -> find "nRW"
  | LExpr_Field (LExpr_Var (Ident "PSTATE"), Ident "N") -> find "NF"
  | LExpr_Field (LExpr_Var (Ident "PSTATE"), Ident "Z") -> find "ZF"
  | LExpr_Field (LExpr_Var (Ident "PSTATE"), Ident "C") -> find "CF"
  | LExpr_Field (LExpr_Var (Ident "PSTATE"), Ident "V") -> find "VF"

  | LExpr_Array _
  | LExpr_Wildcard
  | LExpr_Field (_, _)
  | LExpr_Fields (_, _)
  | LExpr_Slices (_, _)
  | LExpr_BitTuple _
  | LExpr_Tuple _
  | LExpr_Write (_, _, _)
  | LExpr_ReadWrite (_, _, _, _) -> err "unsupported"



(** a tuple of entry and exit basic blocks. 

    has the following properties: 
    - the two blocks are linked, with the entry block 
      suitable for entering the concept described
    - the exit block does not have a terminating instruction
    - the two blocks may refer to the same block if there is only one.
 *)
type blockpair = llbasicblock * llbasicblock

let link (first: blockpair) (second: blockpair): blockpair = 
    let (b1, b2) = first in
    let (rest1, rest2) = second in
    ignore @@ build_br rest1 (builder_at_end ctx b2);
    (b1, rest2)

let rec translate_stmt (stmt : LibASL.Asl_ast.stmt) : llbasicblock * llbasicblock = 
  let entry = append_block ctx "stmt" func in
  let build = builder_at_end ctx entry in
  let single = (entry, entry) in

  match stmt with 
  | Stmt_Assert _ -> single
  | Stmt_VarDeclsNoInit (ty, nms, _) -> 
    let ty = translate_ty ty in
    List.iter (fun nm -> 
      let alloc = build_alloca ty (ident nm) build in
      vars := Globals.add (ident nm) alloc !vars;
    ) nms;
    single

  | Stmt_VarDecl (ty, nm, rv, _)  
  | Stmt_ConstDecl (ty, nm, rv, _) ->
    let ty = translate_ty ty in
    let alloc = build_alloca ty (ident nm) build in
    ignore @@ build_store (translate_expr rv build) alloc build;

    vars := Globals.add (ident nm) alloc !vars;
    single
  | Stmt_Assign (le, rv, _) -> 
    let rv = translate_expr rv build in
    ignore @@ build_store rv (translate_lexpr le) build;
    single
  | Stmt_Throw (_, _) -> assert false
  | Stmt_TCall (_, _, _, _) -> assert false
  | Stmt_If (cond, trues, elses, falses, _) -> 
    assert (elses = []);

    let cond = translate_expr cond build in

    let t_hd,t_tl = translate_stmts trues in 
    let f_hd,f_tl = translate_stmts falses in

    ignore @@ build_cond_br cond t_hd f_hd build;

    let exit = append_block ctx "if_merge" func in
    ignore @@ build_br exit (builder_at_end ctx t_tl);
    ignore @@ build_br exit (builder_at_end ctx f_tl);

    entry,exit
    
  
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


and translate_stmts (stmts : stmt list) : llbasicblock * llbasicblock = 
  match stmts with 
  | [] -> 
    let b = append_block ctx "" func in b,b
  | s::rest -> 
    let s' = translate_stmt s in 
    let rest' = translate_stmts rest in 
    link s' rest'


let branchtaken_fix (build: llbuilder) : unit =
  let pc_ptr = Globals.find "PC" !vars in

  let branchtaken_ptr = Globals.find "__BranchTaken" !vars in
  let bt = build_load branchtaken_ptr "" build in
  let pc = build_load pc_ptr "" build in
  let inc = build_add pc (const_int (type_of pc) 4) "" build in
  let pc' = build_select bt pc inc "" build in
  ignore @@ build_store pc' pc_ptr build


let translate_stmts_entry (stmts: stmt list) = 
  vars := fold_left_globals (fun bs g -> Globals.add (value_name g) g bs) Globals.empty llmodule;

  let entry = entry_block func in
  let entry_builder = builder_at_end ctx entry in

  let branchtaken = build_alloca (i1_type ctx) "__BranchTaken" entry_builder in
  let locals = List.to_seq [
    branchtaken;
    build_alloca (integer_type ctx 2) "BTypeNext" entry_builder;
    build_alloca bool_t "nRW" entry_builder;
  ]
  in  
  vars := Globals.add_seq (Seq.map (fun x -> (value_name x, x)) locals) !vars;

  ignore @@ build_store bool_false branchtaken entry_builder;
  let (hd,tl) = translate_stmts stmts in

  ignore @@ build_br hd entry_builder;
  let tl_builder = builder_at_end ctx tl in
  branchtaken_fix tl_builder;
  ignore @@ build_ret_void tl_builder;

  hd,tl
  

  
let load_aslb f = 
  let chan = open_in_bin f in 
  let stmts : LibASL.Asl_ast.stmt list = Marshal.from_channel chan in
  (* List.iter (fun x -> print_endline (LibASL.Asl_utils.pp_stmt x)) stmts *)
  stmts
