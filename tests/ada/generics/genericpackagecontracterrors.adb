procedure GenericPackageContractErrors is
    function Next (Value : Integer) return Integer is
    begin
        return Value + 1;
    end Next;
    function Different (Value : Integer) return Integer is
    begin
        return Value + 1;
    end Different;
    Left : Integer := 0;
    Right : Integer := 0;
    generic
        N : Integer;
        with function Step (Value : Integer) return Integer;
        Target : in out Integer;
    package Model is end Model;
    package Wrong_Number is new Model (4, Next, Left);
    package Wrong_Function is new Model (3, Different, Left);
    package Wrong_Object is new Model (3, Next, Right);
    generic
        with package P is new Model (3, Next, Left);
    package Consumer is end Consumer;
    package Number_Mismatch is new Consumer (Wrong_Number);
    package Function_Mismatch is new Consumer (Wrong_Function);
    package Object_Mismatch is new Consumer (Wrong_Object);
    generic
        N : Integer;
    package Values is end Values;
    package Variable_Value is new Values (Left);
    generic
        with package P is new Values (Left);
    package Variables_Do_Not_Match is end Variables_Do_Not_Match;
    package Same_Variable is new Variables_Do_Not_Match (Variable_Value);
    function Runtime return Integer is
    begin
        return 3;
    end Runtime;
    Constant_1 : constant Integer := Runtime;
    Constant_2 : constant Integer := Runtime;
    package Constant_Value is new Values (Constant_1);
    generic
        with package P is new Values (Constant_2);
    package Distinct_Constants is end Distinct_Constants;
    package Same_Runtime_Value is new Distinct_Constants (Constant_Value);
begin
    null;
end GenericPackageContractErrors;
