/**
 * This file is part of Poster Safari.
 *
 *  Poster Safari is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Poster Safari is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with Poster Safari.  If not, see <http://www.gnu.org/licenses/>.
 * 
 * 
 *  Copyright 2017 Moritz Dannehl, Rebecca Lühmann, Nicolas Marin, Michael Nieß, Julian Wolff
 * 
 */

#ifndef WORKER_EXPORT_H
#define WORKER_EXPORT_H

#ifdef WORKER_LIBRARY

#include <functional>

typedef std::function<void(const char*, int)>* WorkerCallbackSerialized;
        
Postr::_WORKER_CLASS_ *_worker = nullptr;
WorkerCallbackSerialized _serialcallback = nullptr;

void _allocate()
{
    _worker = new Postr::_WORKER_CLASS_();
}

void _free()
{
    delete _worker;
}

void _callback(Postr::Data data, int status)
{
    if(_serialcallback)
        (*_serialcallback)(data.serialize(true).data(), status);
    _serialcallback = nullptr;
}

int _process(const char *serialdata, void *callback = 0)
{
    _serialcallback = (WorkerCallbackSerialized)callback;
    Postr::Data data(serialdata);
    return _worker->process(data, _callback);
}

float _progress()
{
    return _worker->progress();
}

extern "C" 
{
    WORKER_EXPORTS void freeWorker()
    {
        _free();
    }

    WORKER_EXPORTS int process(const char *serialdata, void *callback = 0)
    {
        if(!_worker)
            _allocate();
        _process(serialdata, callback);
    }
    
    WORKER_EXPORTS float progress()
    {
        if(!_worker)
            _allocate();
        return _progress();
    }
    
    WORKER_EXPORTS const char *name()
    {
        return _WORKER_NAME_;
    }

}
#endif //WORKER_LIBRARY

#endif //WORKER_EXPORT_H
