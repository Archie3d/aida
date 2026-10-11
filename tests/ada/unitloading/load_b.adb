with Load_A;
package body Load_B is
    function Read_Value return Integer is
    begin
        return Load_A.Value;
    end Read_Value;
end Load_B;
