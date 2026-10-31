procedure Callbackerrors is
    type Function_Access is access function (X : Integer) return Integer;
    type Procedure_Access is access procedure (X : in out Integer);
    function Wrong (X : Float) return Integer is
    begin
        return 0;
    end Wrong;
    procedure Wrong_Mode (X : out Integer) is
    begin
        X := 0;
    end Wrong_Mode;
    F : Function_Access := Wrong'Access;
    P : Procedure_Access := Wrong_Mode'Access;
    procedure Inner is
        function Too_Deep (X : Integer) return Integer is
        begin
            return X;
        end Too_Deep;
    begin
        F := Too_Deep'Access;
    end Inner;
begin
    F := P;
    F (1);
    P (1);
    F (1) := 2;
    F (Missing => 1);
    F (1, 2);
end Callbackerrors;
