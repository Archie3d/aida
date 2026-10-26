procedure GenericAccessContractErrors is
    generic
        type Item is private;
        type P is access Item;
    procedure Bad (Value : P);
    procedure Bad (Value : P) is
    begin
        Value.all := Value.all + 1;
    end Bad;
    generic
        type P is access constant Integer;
    procedure Write (Value : P);
    procedure Write (Value : P) is
    begin
        Value.all := 1;
    end Write;
    generic
        type P is access procedure (X : in out Integer);
    procedure Wrong_Call (Call : P);
    procedure Wrong_Call (Call : P) is
    begin
        Call (1);
    end Wrong_Call;
    generic
        type P is access procedure (X : Integer := 0);
    package Default_Parameter is
    end Default_Parameter;
    generic
        type P is access function (X : out Integer) return Integer;
    package Function_Mode is
    end Function_Mode;
begin
    null;
end GenericAccessContractErrors;
