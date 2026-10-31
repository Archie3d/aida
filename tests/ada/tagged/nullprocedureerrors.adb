procedure NullProcedureErrors is
    procedure Complete is null;
    procedure Complete is
    begin
        null;
    end Complete;
    procedure Wrong_Name (Value : Integer);
    procedure Wrong_Name (Other : Integer) is null;
    procedure Wrong_Mode (Value : in out Integer);
    procedure Wrong_Mode (Value : Integer) is null;
    procedure Wrong_Default (Value : Integer := 1);
    procedure Wrong_Default (Value : Integer := 2) is null;
begin
    null;
end NullProcedureErrors;
