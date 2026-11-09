package Ada.Task_Identification is
    pragma Preelaborate (Task_Identification);
    type Task_Id is private;
    Null_Task_Id : constant Task_Id;

    function "=" (Left, Right : Task_Id) return Boolean;
    function Image (T : Task_Id) return String;
    pragma Import (C, Image, "__ada_task_image");
    function Current_Task return Task_Id;
    pragma Import (C, Current_Task, "__ada_current_task");
    function Is_Terminated (T : Task_Id) return Boolean;
    pragma Import (C, Is_Terminated, "__ada_task_is_terminated");
    function Is_Callable (T : Task_Id) return Boolean;
    pragma Import (C, Is_Callable, "__ada_task_is_callable");
    -- Abort_Task is deferred with task abort to Phase 9.
private
    type Task_Id is access Integer;
    Null_Task_Id : constant Task_Id := null;
end Ada.Task_Identification;
