package body Return_Model is
    function Make (Value : Integer) return Cell is
    begin
        return Result : aliased Cell do
            Result.Value := Value;
            Result.Self := Result.Value'Unchecked_Access;
        end return;
    end Make;
    function Forward (Value : Integer) return Cell is
    begin
        return Make (Value);
    end Forward;
    function Many (Count : Natural) return Cells is
    begin
        return Result : Cells (3 .. Count + 2) do
            for I in Result'Range loop
                Result (I).Value := I;
                Result (I).Self := Result (I).Value'Unchecked_Access;
            end loop;
        end return;
    end Many;
    function Make_Pair return Pair is
    begin
        return (First => Forward (5), Second => Make (6));
    end Make_Pair;
    function Make_Buffer (Count : Natural) return Buffer is
    begin
        return Result : Buffer (Count) do
            Result.Text := (others => 'z');
            Result.Item.Value := Count;
            Result.Item.Self := Result.Item.Value'Unchecked_Access;
        end return;
    end Make_Buffer;
end Return_Model;
