package ElaborationBlockErrors is
end ElaborationBlockErrors;
package body ElaborationBlockErrors is
begin
    declare
        Local : Integer := 1;
    begin
        return;
    end;
    Local := 2;
exception
    when others =>
        begin
            return;
        end;
end ElaborationBlockErrors;
