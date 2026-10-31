with Ada.Text_IO;

procedure ExternalLink is
    function Answer return Integer;
    pragma Import (C, Answer, "__ada_test_external_answer");
begin
    if Answer /= 42 then
        raise Program_Error;
    end if;
    Ada.Text_IO.Put_Line ("external linkage passed");
end ExternalLink;
