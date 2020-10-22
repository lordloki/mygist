void KX_Scene::ConvertBlenderObjectsList(std::vector<Object> *objectlist, bool asynchronous)
{
  if (asynchronous) {
    TaskPool *taskpool = BLI_task_pool_create(nullptr, TASK_PRIORITY_LOW);

    /* Convert the Blender collection in a different thread, so that the
     * game engine can keep running at full speed. */
    ConvertBlenderObjectsListTaskData *task = (ConvertBlenderObjectsListTaskData *)MEM_mallocN(sizeof(ConvertBlenderObjectsListTaskData),
                                                                                             "convertblenderobjectslist-data");

    task->engine = KX_GetActiveEngine();
    task->physics_engine = UseBullet;
    task->co = co;
    task->scene = this;
    task->converter = m_sceneConverter;

    BLI_task_pool_push(taskpool,
                       (TaskRunFunction)convert_blender_objects_list_thread_func,
                       task,
                       true,  // free task data
                       NULL);
    BLI_task_pool_work_and_wait(taskpool);
    BLI_task_pool_free(taskpool);
    taskpool = nullptr;
  }
  else {
    convert_blender_objects_list_synchronous(co);
  }
}



KX_PYMETHODDEF_DOC(KX_Scene,
                   convertBlenderObjectsList,
                   "convertBlenderObjectsList()\n"
                   "\n")
{
  std:vector<Object> objectlist;
  PyObject *list;
  int asynchronous;

  if (!PyArg_ParseTuple(args, "O!i:", &PyList_Type, &list, &asynchronous)) {
    std::cout << "Expected a bpy.types.Object list." << std::endl;
    return nullptr;
  }
  
  std:vector<Object> objectslist;
  
  for (Py_ssize_t i = 0; i < list_size; i++) {
    PyObject* bl_object = PyList_GetItem(list, i);

    ID *id;
    if (!pyrna_id_FromPyObject(bl_object, &id)) {
      std::cout << "Failed to convert object." << std::endl;
      return nullptr;
    }

    objectslist.push_back((Object *)id);
  }

  ConvertBlenderObjectsList(objectlist, asynchronous);
  Py_RETURN_NONE;
}
