pragma Locking_Policy (Ceiling_Locking);
package CeilingModel is
    protected Object is
        function Level return Integer;
    end Object;
end CeilingModel;
