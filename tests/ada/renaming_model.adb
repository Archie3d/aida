package body Renaming_Model is
    function Factory return Integer is
    begin
        return 9;
    end Factory;
    procedure Increase is
    begin
        Value := Value + 1;
    end Increase;
begin
    Value := Value + 10;
end Renaming_Model;
