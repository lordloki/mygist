PyObject *SCA_PythonMouse::pyattr_get_active_inputs(PyObjectPlus *self_v,
                                                    const KX_PYATTRIBUTE_DEF *attrdef)
{
  SCA_PythonMouse *self = static_cast<SCA_PythonMouse *>(self_v);

  PyDict_Clear(self->m_event_dict);

  for (int i = SCA_IInputDevice::BEGINMOUSE; i <= SCA_IInputDevice::ENDMOUSE; i++) {
    SCA_InputEvent &input = self->m_mouse->GetInput((SCA_IInputDevice::SCA_EnumInputs)i);

    unsigned int num_active = input.Find(SCA_InputEvent::ACTIVE);
    if (num_active) {
      PyObject *key = PyLong_FromLong(i);
        
      for (int i = 0; i < num_active; i++) {
        // Generate a input event with one of the accumulative events only
        SCA_InputEvent temp;
        temp.m_status.push_back(input.m_status[i+1]);
        temp.m_values.push_back(input.m_values[i+1]);
        temp.m_queue.push_back(input.m_queue[i]);
	  
        PyDict_SetItem(self->m_event_dict, key, temp.GetProxy());
	
        Py_DECREF(key);
        Py_INCREF(self->m_event_dict);
      }
    }
  }
  return self->m_event_dict;
}
