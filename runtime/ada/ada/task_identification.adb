package body Ada.Task_Identification is
    function Equal_Ids (Left, Right : Task_Id) return Boolean;
    pragma Import (C, Equal_Ids, "__ada_task_id_equal");

    function "=" (Left, Right : Task_Id) return Boolean is
    begin
        return Equal_Ids (Left, Right);
    end "=";
end Ada.Task_Identification;
