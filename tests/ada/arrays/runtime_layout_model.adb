package body Runtime_Layout_Model is
    protected body Counter is
        procedure Increment is
        begin
            if Data /= "ab" then raise Program_Error; end if;
            Count := Count + 1;
        end Increment;
        function Value return Integer is
        begin
            return Count;
        end Value;
    end Counter;
    function Make return Holder is
    begin
        return Global;
    end Make;
    function Make_Hidden return Hidden is
    begin
        return Hidden_Value;
    end Make_Hidden;
    procedure Check_Hidden (X : Hidden) is
    begin
        if X.Data /= "hhh" or X.Last /= 42 then raise Program_Error; end if;
    end Check_Hidden;
begin
    N := 100;
end Runtime_Layout_Model;
