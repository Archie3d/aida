with Ada.Dynamic_Priorities; use Ada.Dynamic_Priorities;
package body TaskPriorityLibrary is
    task body Worker is
    begin
        if Get_Priority /= 14 then raise Program_Error; end if;
        accept Check;
    end Worker;
end TaskPriorityLibrary;
