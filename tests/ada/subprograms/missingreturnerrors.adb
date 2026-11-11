procedure Missingreturnerrors is
    function Empty return Integer is
    begin
        null;
    end Empty;
    function Nested_Only return Integer is
        function Inner return Integer is
        begin
            return 1;
        end Inner;
    begin
        null;
    end Nested_Only;
    function Raises_Only return Integer is
    begin
        raise Constraint_Error;
    end Raises_Only;
    function Package_Only return Integer is
        package Helper is
            function Value return Integer;
        end Helper;
        package body Helper is
            function Value return Integer is
            begin
                return 2;
            end Value;
        end Helper;
    begin
        null;
    end Package_Only;
begin
    null;
end Missingreturnerrors;
