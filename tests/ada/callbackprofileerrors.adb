procedure Callbackprofileerrors is
    type Defaults is access procedure (X : Integer := 1);
    type Function_Mode is access function (X : out Integer) return Integer;
    type Duplicate is access procedure (X : Integer; X : Float);
    type Getter is access function return Integer;
    function Get return Integer is
    begin
        return 1;
    end Get;
    G : Getter := Get'Access;
begin
    G.all := 2;
end Callbackprofileerrors;
