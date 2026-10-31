procedure GenericAccessErrors is
    subtype Small is Integer range 1 .. 3;
    type Pool is access Integer;
    type General is access all Integer;
    type Read_Only is access constant Integer;
    type Different is access Float;
    type Restricted is access Small;
    subtype Nonnull is not null General;
    type Proc is access procedure (X : in out Integer);
    type Wrong_Mode is access procedure (X : Integer);
    type Wrong_Type is access procedure (X : in out Float);
    type Func is access function (X : Integer) return Integer;
    type Wrong_Result is access function (X : Integer) return Float;
    type Extra is access procedure (X : in out Integer; Y : Integer);
    generic
        type P is access Integer;
    package Pool_Formal is
    end Pool_Formal;
    generic
        type P is access all Integer;
    package General_Formal is
    end General_Formal;
    generic
        type P is access constant Integer;
    package Constant_Formal is
    end Constant_Formal;
    generic
        type P is not null access all Integer;
    package Nonnull_Formal is
    end Nonnull_Formal;
    generic
        type P is access procedure (X : in out Integer);
    package Procedure_Formal is
    end Procedure_Formal;
    generic
        type P is access function (X : Integer) return Integer;
    package Function_Formal is
    end Function_Formal;
    package Bad_Kind is new Pool_Formal (Integer);
    package Bad_Target is new Pool_Formal (Different);
    package Bad_Constraint is new Pool_Formal (Restricted);
    package Bad_Constant is new Pool_Formal (Read_Only);
    package Bad_All is new General_Formal (Pool);
    package Bad_Mutable is new Constant_Formal (General);
    package Bad_Null is new Nonnull_Formal (General);
    package Bad_Nonnull is new General_Formal (Nonnull);
    package Bad_Profile_Kind is new Procedure_Formal (Pool);
    package Bad_Mode is new Procedure_Formal (Wrong_Mode);
    package Bad_Parameter is new Procedure_Formal (Wrong_Type);
    package Bad_Result is new Function_Formal (Wrong_Result);
    package Bad_Count is new Procedure_Formal (Extra);
    package Bad_Function is new Function_Formal (Proc);
    package Good_Pool is new Pool_Formal (Pool);
    package Good_General is new Pool_Formal (General);
    package Good_Procedure is new Procedure_Formal (Proc);
    package Good_Function is new Function_Formal (Func);
    package Good_Constant is new Constant_Formal (Read_Only);
    package Good_Nonnull is new Nonnull_Formal (Nonnull);
begin
    null;
end GenericAccessErrors;
