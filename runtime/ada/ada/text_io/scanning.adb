package body Ada.Text_IO.Scanning is
    function Normalized (Text : String) return String is
        result : String := Text;
    begin
        for i in result'Range loop
            if result (i) = Character'Val (9) then
                result (i) := ' ';
            end if;
        end loop;
        return result;
    end Normalized;
end Ada.Text_IO.Scanning;
