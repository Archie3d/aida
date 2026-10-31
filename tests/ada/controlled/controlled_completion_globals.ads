with Controlled_Completion_Model; use Controlled_Completion_Model;
package Controlled_Completion_Globals is
    Last : Audit;
    Limited_Object : Limited_Guard := Make;
    Dynamic_Object : Root'Class := Dynamic;
    type Link is access Root'Class;
    Heap : Link := new Root'Class'(Dynamic);
end Controlled_Completion_Globals;
